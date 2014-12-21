/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/app/application.h>
#include <et/core/conversion.h>
#include <et/core/filesystem.h>
#include <et/apiobjects/vertexbuffer.h>
#include <et/models/objloader.h>

using namespace et;
using namespace et::s3d;

namespace et
{
	class OBJLoaderThread : public Thread
	{
	public:
		OBJLoaderThread(OBJLoader*, ObjectsCache&);
		
		ThreadResult main();

	private:
		friend class OBJLoader;
		OBJLoader* _owner;
		ObjectsCache& _cache;
	};
}

inline std::istream& operator >> (std::istream& stream, vec2& value)
{
	std::string ln;
	std::getline(stream, ln);
	trim(ln);
	
	auto values = split(ln, " ");
	
	size_t comp = 0;
	for (const auto& val : values)
	{
		if (comp < 2)
			value[comp++] = strToFloat(val);
	}
	
	return stream;
}

inline std::istream& operator >> (std::istream& stream, vec3& value)
{
	std::string ln;
	std::getline(stream, ln);
	trim(ln);
	
	auto values = split(ln, " ");
	
	size_t comp = 0;
	for (const auto& val : values)
	{
		if ((comp < 3) && !val.empty())
			value[comp++] = strToFloat(val);
	}
	
	return stream;
}

inline std::istream& operator >> (std::istream& stream, vec4& value)
{
	std::string ln;
	std::getline(stream, ln);
	trim(ln);
	
	auto values = split(ln, " ");
	
	size_t comp = 0;
	for (const auto& val : values)
	{
		if (comp < 4)
			value[comp++] = strToFloat(val);
	}
	
	return stream;
}

void getLine(std::ifstream& stream, std::string& line);

OBJLoaderThread::OBJLoaderThread(OBJLoader* owner, ObjectsCache& cache) : 
	Thread(false), _owner(owner), _cache(cache)
{
	run();
}

ThreadResult OBJLoaderThread::main()
{
	_owner->loadData(true, _cache);

	_owner->processLoadedData();

	Invocation i;
	i.setTarget(_owner, &OBJLoader::threadFinished);
	i.invokeInMainRunLoop();

	return 0;
}

/*
 * OBJLoader
 */

OBJLoader::OBJLoader(RenderContext* rc, const std::string& inFile) : _rc(rc),
	inputFileName(application().resolveFileName(inFile).c_str()),
	inputFile(inputFileName.c_str()), lastGroup(0), _loadOptions(0)
{
	inputFilePath = getFilePath(inputFileName);
	
	if (inputFile.fail())
		log::info("Unable to open file %s", inputFileName.c_str());
}

OBJLoader::~OBJLoader()
{
	if (_thread.valid())
	{
		_thread->stop();
		_thread->waitForTermination();
	}
	
	for (auto group : _groups)
		sharedObjectFactory().deleteObject(group);
	
	_groups.clear();
	
	if (inputFile.is_open())
		inputFile.close();
	
	if (materialFile.is_open())
		materialFile.close();
}

void OBJLoader::loadData(bool async, ObjectsCache& cache)
{
	std::string line;
	int lineNumber = 0;
	char key = 0;
	while (inputFile.is_open() && !inputFile.eof())
	{
		++lineNumber;
		inputFile >> key;
		
		if (inputFile.eof()) 
			break;
		
		key = static_cast<char>(tolower(key));
		
		if (key == '#')
		{
			getLine(inputFile, line);
		}
		else if (key == 'm') // mtllib
		{
			std::string mtllib;
			inputFile >> mtllib;
			lowercase(mtllib);
			
			if (mtllib.compare("tllib") == 0)
			{
				std::string matName;
				inputFile >> matName;
				loadMaterials(matName, async, cache);
			}
			else
			{
				std::cout << "Unresolved symbol: " << key << mtllib << std::endl;
			}
		}
		else if (key == 'g') // group
		{
			getLine(inputFile, line);
			trim(line);
			
			lastGroup = sharedObjectFactory().createObject<OBJGroup>(line);
			_groups.push_back(lastGroup);
		}
		else if (key == 'u') // group's material
		{
			std::string usemtl;
			inputFile >> usemtl;
			lowercase(usemtl);
			
			if (usemtl.compare("semtl") == 0) 
			{
				std::string materialId;
				inputFile >> materialId;
				
				if ((lastGroup != nullptr) && (lastGroup->material.empty() || lastGroup->material == materialId))
				{
					lastGroup->material = materialId;
				}
				else
				{
					auto groupName = "group-" + intToStr(_lastGroupId++) + "-" + materialId;
					lastGroup = sharedObjectFactory().createObject<OBJGroup>(groupName, materialId);
					_groups.push_back(lastGroup);
				}
			}
			else
			{
				std::cout << "Unresolved symbol " << key << usemtl << ". Current group: " << lastGroup->name << std::endl;
			}
		}
		else if (key == 's') // smoothing group
		{
			if (lastGroup == nullptr)
			{
				lastGroup = sharedObjectFactory().createObject<OBJGroup>("group-" + intToStr(_lastGroupId++));
				_groups.push_back(lastGroup);
			}
			getLine(inputFile, line);
			_lastSmoothGroup = (line.compare("off") == 0) ? 0 : strToInt(line);
		}
		else if (key == 'f') // faces
		{
			OBJFace face;
			std::getline(inputFile, line);
			trim(line);

			auto faces = split(line, " ");
			ET_ASSERT((faces.size() > 2) && (faces.size() < 6));

			for (auto inFace : faces)
			{
				OBJVertex vertex;
				auto indexes = split(inFace, "/");
								
				vertex.numVertices = indexes.size();

				size_t i = 0;
				for (auto index : indexes)
				{
					auto iValue = strToInt(index);
					
					if (iValue < 0)
					{
						size_t szValue = static_cast<size_t>(-iValue);
						ET_ASSERT(szValue <= _vertices.size());
						vertex[i++] = _vertices.size() - szValue;
					}
					else
					{
						vertex[i++] = iValue - 1;
					}
				}
				
				face.vertices.push_back(vertex);
			}
			
			if (lastGroup == nullptr)
			{
				lastGroup = sharedObjectFactory().createObject<OBJGroup>("group-" + intToStr(_lastGroupId++));
				_groups.push_back(lastGroup);
			}
			
			lastGroup->faces.push_back(face);
		}
		else if (key == 'v')
		{
			char subKey = static_cast<char>(inputFile.peek());
			
			if (subKey == 't')
			{
				vec2 vertex;
				inputFile >> subKey >> vertex;
				_texCoords.push_back(vertex);
			}
			else if (subKey == 'n')
			{
				vec3 vertex;
				inputFile >> subKey >> vertex;

				if ((_loadOptions & Option_SwapYwithZ) == Option_SwapYwithZ)
					std::swap(vertex.y, vertex.z);

				_normals.push_back(vertex);
			}
			else if (isWhitespaceChar(subKey))
			{
				vec3 vertex;
				inputFile >> vertex;

				if ((_loadOptions & Option_SwapYwithZ) == Option_SwapYwithZ)
					std::swap(vertex.y, vertex.z);

				_vertices.push_back(vertex);
			}
			else
			{
				getLine(inputFile, line);
				std::cout << "Unknown line " << lineNumber << " in file: " << line << std::endl;
			}
		}
		else
		{
			getLine(inputFile, line);
			std::cout << "Unknown line " << lineNumber << " in file: " << key << line << std::endl;
		}
		
	}
}

s3d::ElementContainer::Pointer OBJLoader::load(ObjectsCache& cache, size_t options)
{
	_loadOptions = options;

	loadData(false, cache);
	
	processLoadedData();

	s3d::ElementContainer::Pointer result = generateVertexBuffers();
	loaded.invoke(result);
	return result;
}

void OBJLoader::loadAsync(ObjectsCache& cache)
{
	_thread = sharedObjectFactory().createObject<OBJLoaderThread>(this, cache);
}

void OBJLoader::loadMaterials(const std::string& fileName, bool async, ObjectsCache& cache)
{
	application().pushSearchPath(inputFilePath);
	std::string filePath = application().resolveFileName(fileName);
	
	materialFile.open(filePath.c_str());
	if (!materialFile.is_open())
	{
		std::cout << "Could not open " << filePath;
		return;
	}
	
	char key = 0;
	std::string line;
	
	while (!materialFile.eof())
	{
		materialFile >> key;
		
		if (materialFile.eof())
			break;
		
		key = static_cast<char>(tolower(key));
		
		if (key == '#')
		{
			getLine(materialFile, line);
		}
		else if (key == 'b')
		{
			std::string bump;
			materialFile >> bump;
			lowercase(bump);
			
			if (bump.compare("ump") == 0)
			{
				char next = static_cast<char>(materialFile.peek());
				
				while (next == ' ') 
				{
					materialFile >> next;
					materialFile.putback(next);
				}
				
				if (next == '-')
				{
					std::string bumpId;
					materialFile >> bumpId;
					lowercase(bumpId);
					if (bumpId.compare("-bm") == 0)
					{
						float value = 0.0f;
						materialFile >> value;
						_lastMaterial->setFloat(MaterialParameter_BumpFactor, value);
						
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
					}
					else
					{
						getLine(materialFile, line);
						std::cout << "Unresolved (unsupported) bump map:\n" << key << bump << bumpId << line << std::endl;
					}
				}
				else
				{
					getLine(materialFile, line);
					_lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
				}
			}
			else
			{
				getLine(materialFile, line);
				std::cout << "Unknown (unsupported) line in material: " << key << bump << line << std::endl;
			}
		}
		else if (key == 'k')
		{
			if (_lastMaterial.invalid())
			{
				getLine(materialFile, line);
			}
			else
			{
				char next = 0;
				materialFile >> next;
				next = static_cast<char>(tolower(next));
				
				if (next == 'a')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter_AmbientColor, value);
				} 
				else if (next == 'd')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter_DiffuseColor, value);
				} 
				else if (next == 's')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter_SpecularColor, value);
				} 
				else if (next == 'e')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter_EmissiveColor, value);
				} 
				else
				{
					getLine(materialFile, line);
					std::cout << "Unresolved (unsupported) material parameter:\n" << key << next << line << std::endl;
				}
			}
		}
		else if (key == 't')
		{
			char next = 0;
			materialFile >> next;
			next = static_cast<char>(tolower(next));
			
			if (next == 'r')
			{
				float value = 0.0f;
				materialFile >> value;
				_lastMaterial->setFloat(MaterialParameter_Transparency, value);
			}
			else if (next == 'f') // skip
			{
				getLine(materialFile, line);
			}
			else
			{
				getLine(materialFile, line);
				std::cout << "Unresolved (unsupported) transparency:\n" << key << next << line << std::endl;
			}
		}
		else if (key == 'm')
		{
			char map[4] = { };
			materialFile >> map[0] >> map[1] >> map[2];
			
			if (strcmp(map, "ap_") == 0)
			{
				char mapId = 0;
				materialFile >> mapId;
				mapId = static_cast<char>(tolower(mapId));
				
				if (mapId == 'k')
				{
					char subId;
					materialFile >> subId;
					subId = static_cast<char>(tolower(subId));
					
					if (subId == 'd')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter_DiffuseMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'a')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter_AmbientMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 's')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter_SpecularMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'e')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter_EmissiveMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else
					{
						getLine(materialFile, line);
						std::cout << "Unresolved (unsupported) map:\n" << key << map << mapId << line << std::endl;
					}
					
				}
				else if (mapId == 'b')
				{
					std::string bump;
					materialFile >> bump;
					lowercase(bump);
					
					if (bump.compare("ump") == 0)
					{
						char next = static_cast<char>(materialFile.peek());
						
						while (next == ' ') 
						{
							materialFile >> next;
							materialFile.putback(next);
						}
						
						if (next == '-')
						{
							std::string bumpId;
							materialFile >> bumpId;
							lowercase(bumpId);
							if (bumpId.compare("-bm") == 0)
							{
								float value;
								materialFile >> value;
								_lastMaterial->setFloat(MaterialParameter_BumpFactor, value);
								
								getLine(materialFile, line);
								_lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
							}
							else
							{
								getLine(materialFile, line);
								std::cout << "Unresolved (unsupported) bump map:\n" << mapId << bump << bumpId << line << std::endl;
							}
						}
						else
						{
							getLine(materialFile, line);
							_lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
						}
					}
					else
					{
						std::cout << "Unknown map type: " << mapId << bump << std::endl;
					}
				}
				else if (mapId == 'd')
				{
					getLine(materialFile, line);
					_lastMaterial->setTexture(MaterialParameter_TransparencyMap, _rc->textureFactory().loadTexture(line, cache, async) );
				}
				else
				{
					std::cout << "Unknown map type: " << mapId << std::endl;
				}
				
			}
			else
			{
				getLine(materialFile, line);
				std::cout << "Unresolved (unsupported) line:\n" << key << map << line << std::endl;
			}
			
		}
		else if (key == 'd')
		{
			getLine(materialFile, line);
		}
		else if (key == 'i')
		{
			if (_lastMaterial.invalid())
			{
				getLine(materialFile, line);
			}
			else
			{
				std::string illum;
				materialFile >> illum;
				lowercase(illum);
				if (illum == "llum")
				{              
					int value = 0;
					materialFile >> value;
					_lastMaterial->setInt(MaterialParameter_ShadingModel, value);
				}
				else
				{
					std::cout << "Unresolved symbol: " << key << illum << std::endl;
				}
			}
		}
		else if (key == 'n')
		{
			char next = 0;
			materialFile >> next;
			next = static_cast<char>(tolower(next));
			
			if (next == 's')
			{
				float value = 0.0f;
				materialFile >> value;
				_lastMaterial->setFloat(MaterialParameter_Roughness, value);
			}
			else if (next == 'i')
			{
				getLine(materialFile, line);
			}
			else if (next == 'e')
			{
				std::string newmtl;
				materialFile >> newmtl;
				lowercase(newmtl);
				
				if (newmtl == "wmtl")
				{
					std::string name;
					materialFile >> name;
					
					_lastMaterial = Material::Pointer();
					_lastMaterial->setName(name);
					
					_materials.push_back(_lastMaterial);
				}
				else
				{
					std::cout << "Unresolved symbol: " << key << newmtl << std::endl;
				}
				
			}
			else
			{
				getLine(materialFile, line);
				std::cout << "Unresolved (unsupported) line:\n" << key << line << std::endl;
			}
			
		}
		else
		{
			getLine(materialFile, line);
			std::cout << "Unresolved (unsupported) line:\n" << key << line << std::endl;
		}
		
	}
	
	materialFile.close();
	application().popSearchPaths();
}

void OBJLoader::processLoadedData()
{
	size_t totalTriangles = 0;

	for (const auto& group : _groups)
	{
		for (const auto& face : group->faces)
			totalTriangles += face.vertices.size() - 2;
	}
	
	size_t totalVertices = 3 * totalTriangles;
	
	bool hasNormals = _normals.size() > 0;
	bool hasTexCoords = _texCoords.size() > 0;
		
	VertexDeclaration decl(true, Usage_Position, Type_Vec3);
	
	if (hasNormals)
		decl.push_back(Usage_Normal, Type_Vec3);

	if (hasTexCoords)
		decl.push_back(Usage_TexCoord0, Type_Vec2);
		
	IndexArrayFormat fmt = (totalVertices > 65535) ? IndexArrayFormat_32bit : IndexArrayFormat_16bit;
	
	_vertexData = VertexArray::Pointer::create(decl, totalVertices);
	_indices = IndexArray::Pointer::create(fmt, totalVertices, PrimitiveType_Triangles);
	
	_indices->linearize(totalVertices);

	RawDataAcessor<vec3> pos = _vertexData->chunk(Usage_Position).accessData<vec3>(0);
	RawDataAcessor<vec3> norm = _vertexData->chunk(Usage_Normal).accessData<vec3>(0);
	RawDataAcessor<vec2> tex = _vertexData->chunk(Usage_TexCoord0).accessData<vec2>(0);
	
	size_t index = 0;
	
	auto PUSH_VERTEX = [this, &pos, &norm, &tex, &index, hasTexCoords, hasNormals](const OBJVertex& vertex, const vec3& offset)
	{
		{
			ET_ASSERT(vertex[0] < _vertices.size());
			pos[index] = _vertices[vertex[0]] - offset;
		}
		
		if (hasTexCoords)
		{
			ET_ASSERT(vertex[1] < _texCoords.size());
			tex[index] = _texCoords[vertex[1]];
		}
		
		if (hasNormals)
		{
			ET_ASSERT(vertex[2] < _normals.size());
			norm[index] = _normals[vertex[2]];
		}
		
		++index;
	};
	
	for (auto group : _groups)
	{
		IndexType startIndex = static_cast<IndexType>(index);
		
		vec3 center;
		
		if (_loadOptions & Option_CalculateTransforms)
		{
			size_t totalVertices = 0;
			
			for (auto face : group->faces)
			{
				size_t numTriangles = face.vertices.size() - 2;
				for (size_t i = 1; i <= numTriangles; ++i)
				{
					center += _vertices[face.vertices[0][0]];
					center += _vertices[face.vertices[i][0]];
					center += _vertices[face.vertices[i+1][0]];
					totalVertices += 3;
				}
			}
			
			if (totalVertices > 0.0f)
				center /= static_cast<float>(totalVertices);
		}
		
		for (auto face : group->faces)
		{
			size_t numTriangles = face.vertices.size() - 2;
			for (size_t i = 1; i <= numTriangles; ++i)
			{
				PUSH_VERTEX(face.vertices[0], center);
				PUSH_VERTEX(face.vertices[i], center);
				PUSH_VERTEX(face.vertices[i+1], center);
			}
		}
			
		Material::Pointer m;
		
		for (auto mat : _materials)
		{
			if (mat->name() == group->material)
			{
				m = mat;
				break;
			}
		}
			
		_meshes.emplace_back(group->name, startIndex, index - startIndex, m, center);
	}
}

s3d::ElementContainer::Pointer OBJLoader::generateVertexBuffers()
{
	s3d::ElementContainer::Pointer result = s3d::ElementContainer::Pointer::create(inputFileName, nullptr);

	_storage = s3d::Scene3dStorage::Pointer::create(getFileName(inputFileName) + "-storage", result.ptr());
	_storage->addVertexArray(_vertexData);
	_storage->setIndexArray(_indices);
	
	for (auto m : _materials)
		_storage->addMaterial(m);
	
	
	VertexArrayObject vao = _rc->vertexBufferFactory().createVertexArrayObject("model-vao");

	vao->setBuffers(_rc->vertexBufferFactory().createVertexBuffer("model-vb", _vertexData, BufferDrawType_Static),
		_rc->vertexBufferFactory().createIndexBuffer("model-ib", _indices, BufferDrawType_Static));

	for (const auto& i : _meshes)
	{
		s3d::Element::Pointer object;
		
		if (_loadOptions & Option_SupportMeshes)
		{
			auto mesh = SupportMesh::Pointer::create(i.name, vao, i.material, i.start, i.count, result.ptr());
			mesh->fillCollisionData(_vertexData, _indices);
			object = mesh;
		}
		else 
		{
			object = Mesh::Pointer::create(i.name, vao, i.material, i.start, i.count, result.ptr());
		}
		
		object->setTranslation(i.center);
	}

	return result;
}

void OBJLoader::threadFinished()
{
	loaded.invoke(generateVertexBuffers());
}

/*
 * Service
 */
void getLine(std::ifstream& stream, std::string& line)
{
	std::getline(stream, line);
	
	if (line.size() > 0)
	{
		if (line[line.size() - 1] == '\r')
			line.erase(line.end() - 1);
		
		while ((line.size() > 0) && (line[0] == ' '))
			line = line.substr(1, line.size() - 1);
	}
}
