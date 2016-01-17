/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/core/conversion.h>
#include <et/core/filesystem.h>
#include <et/rendering/primitives.h>
#include <et/rendering/material.h>
#include <et/models/objloader.h>

using namespace et;
using namespace et::s3d;

namespace et
{
	class OBJLoaderThread : public Thread
	{
	public:
		OBJLoaderThread(OBJLoader*, s3d::Storage&, ObjectsCache&);
		uint64_t main();
	};
}

template <typename F>
inline void splitAndWrite(const std::string& s, F func)
{
	if (s.empty()) return;
	
	const char t_space = ' ';
	const char t_tab = '\t';

	const char* begin = s.data();
	const char* pos = begin;
	const char* end = begin + s.size();
	while (begin < end)
	{
		if ((*begin == t_space) || (*begin == t_tab))
		{
			func(std::string(pos, begin - pos));
			while ((*begin == t_space) || (*begin == t_tab))
				++begin;
			pos = begin;
		}
		else
		{
			++begin;
		}
	}
	if (begin - pos > 0)
		func(std::string(pos, begin - pos));
}

template <typename F>
inline void splitAndWrite(const std::string& s, char token, F func)
{
	if (s.empty()) return;
	
	const char* begin = s.data();
	const char* pos = begin;
	const char* end = begin + s.size();
	while (begin < end)
	{
		if (*begin == token)
		{
			func(std::string(pos, begin - pos));
			while (*(begin + 1) == token)
			{
				func(intToStr(std::numeric_limits<int>::max()));
				++begin;
			}
			pos = begin + 1;
		}
		++begin;
	}
	if (begin - pos > 0)
		func(std::string(pos, begin - pos));
}

inline std::istream& operator >> (std::istream& stream, vec2& value)
{
	std::string ln;
	ln.reserve(128);
	std::getline(stream, ln);
	trim(ln);

	int comp = 0;
	splitAndWrite(ln, [&value, &comp](const std::string& s)
	{
		if (comp < 2)
			value[comp++] = strToFloat(s);
	});
	return stream;
}

inline std::istream& operator >> (std::istream& stream, vec3& value)
{
	std::string ln;
	ln.reserve(128);
	std::getline(stream, ln);
	trim(ln);

	uint32_t comp = 0;
	splitAndWrite(ln, [&value, &comp](const std::string& s)
	{
		if (comp < 3)
		{
			value[comp++] = strToFloat(s);
		}
	});
	return stream;
}

inline std::istream& operator >> (std::istream& stream, vec4& value)
{
	std::string ln;
	ln.reserve(128);
	std::getline(stream, ln);
	trim(ln);

	uint32_t comp = 0;
	splitAndWrite(ln, [&value, &comp](const std::string& s)
	{
		if (comp < 4)
			value[comp++] = strToFloat(s);
	});
	return stream;
}

void getLine(std::ifstream& stream, std::string& line);

OBJLoaderThread::OBJLoaderThread(OBJLoader*, s3d::Storage&, ObjectsCache&) : 
	Thread(false)
{
	run();
}

uint64_t OBJLoaderThread::main()
{
	ET_FAIL("Not implemented");
	return 0;
}

/*
 * OBJLoader
 */

OBJLoader::OBJLoader(const std::string& inFile, size_t options) :
	inputFileName(application().resolveFileName(inFile).c_str()),
	inputFile(inputFileName.c_str()), lastGroup(nullptr), _loadOptions(options)
{
	inputFilePath = getFilePath(inputFileName);
	
	if (inputFile.fail())
		log::info("Unable to open file %s", inputFileName.c_str());
}

OBJLoader::~OBJLoader()
{
	if (_thread.valid())
		_thread->stopAndWaitForTermination();
	
	for (auto group : _groups)
		etDestroyObject(group);
	
	_groups.clear();
	
	if (inputFile.is_open())
		inputFile.close();
	
	if (materialFile.is_open())
		materialFile.close();
}

void OBJLoader::loadData(bool async, ObjectsCache& cache)
{
	ET_ASSERT(!async && "Async loading is currently disabled");
	
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
			
			lastGroup = etCreateObject<OBJGroup>(line);
			lastGroup->faces.reserve(1024);
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
					lastGroup = etCreateObject<OBJGroup>(groupName, materialId);
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
				lastGroup = etCreateObject<OBJGroup>("group-" + intToStr(_lastGroupId++));
				_groups.push_back(lastGroup);
			}
			getLine(inputFile, line);
			_lastSmoothGroup = (line.compare("off") == 0) ? 0 : strToInt(line);
		}
		else if (key == 'f') // faces
		{
			OBJFace face;
			face.vertices.reserve(1024);
			std::getline(inputFile, line);
			trim(line);

			auto faces = split(line, " ");
			ET_ASSERT((faces.size() > 2) && (faces.size() < 6));

			for (auto inFace : faces)
			{
				OBJVertex vertex;
				vertex.fill(0);

				std::vector<int> indexes;
				indexes.reserve(3);
				splitAndWrite(inFace, '/', [&indexes](const std::string& s)
					{ indexes.push_back(strToInt(s)); });

				uint32_t i = 0;
				for (auto iValue : indexes)
				{
					if (iValue == std::numeric_limits<int>::max())
					{
					}
					else if (iValue < 0)
					{
						size_t szValue = static_cast<size_t>(-iValue);
						if (i == 0)
						{
							ET_ASSERT(szValue <= _vertices.size());
							vertex[i] = _vertices.size() - szValue;
						}
						else if (i == 1)
						{
							ET_ASSERT(szValue <= _texCoords.size());
							vertex[i] = _texCoords.size() - szValue;
						}
						else if (i == 2)
						{
							ET_ASSERT(szValue <= _normals.size());
							vertex[i] = _normals.size() - szValue;
						}
					}
					else
					{
						vertex[i] = iValue - 1;
					}
					++i;
				}
				
				face.vertices.push_back(vertex);
			}
			
			if (lastGroup == nullptr)
			{
				lastGroup = etCreateObject<OBJGroup>("group-" + intToStr(_lastGroupId++));
				_groups.push_back(lastGroup);
			}
			
			lastGroup->faces.push_back(face);
		}
		else if (key == 'v')
		{
			char subKey = static_cast<char>(inputFile.peek());
			
			if (subKey == 't')
			{
				vec2 vertex(0.0f);
				inputFile >> subKey >> vertex;
				_texCoords.push_back(vertex);
			}
			else if (subKey == 'n')
			{
				vec3 vertex(0.0f);
				inputFile >> subKey >> vertex;

				if ((_loadOptions & Option_SwapYwithZ) == Option_SwapYwithZ)
					std::swap(vertex.y, vertex.z);

				_normals.push_back(vertex);
			}
			else if (isWhitespaceChar(subKey))
			{
				vec3 vertex(0.0f);
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

s3d::ElementContainer::Pointer OBJLoader::load(et::RenderContext* rc, MaterialProvider* mp,
	s3d::Storage& storage, ObjectsCache& cache)
{
	storage.flush();

	_rc = rc;
	_materialProvider = mp;
	_groups.reserve(4);
	_vertices.reserve(1024);
	_normals.reserve(1024);
	_texCoords.reserve(1024);

	loadData(false, cache);
	
	processLoadedData();

	s3d::ElementContainer::Pointer result = generateVertexBuffers(storage);
	loaded.invoke(result);
	return result;
}

void OBJLoader::loadAsync(et::RenderContext* rc, s3d::Storage& storage, ObjectsCache& cache)
{
	_rc = rc;
	_thread = etCreateObject<OBJLoaderThread>(this, storage, cache);
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
						_lastMaterial->setFloat(MaterialParameter::BumpFactor, value);
						
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter::NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
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
					_lastMaterial->setTexture(MaterialParameter::NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
					_lastMaterial->setVector(MaterialParameter::AmbientColor, value);
				} 
				else if (next == 'd')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter::DiffuseColor, value);
				} 
				else if (next == 's')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter::SpecularColor, value);
				} 
				else if (next == 'e')
				{
					vec4 value;
					materialFile >> value;
					_lastMaterial->setVector(MaterialParameter::EmissiveColor, value);
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
				_lastMaterial->setFloat(MaterialParameter::Transparency, value);
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
						_lastMaterial->setTexture(MaterialParameter::DiffuseMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'a')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter::AmbientMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 's')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter::SpecularMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'e')
					{
						getLine(materialFile, line);
						_lastMaterial->setTexture(MaterialParameter::EmissiveMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
								_lastMaterial->setFloat(MaterialParameter::BumpFactor, value);
								
								getLine(materialFile, line);
								_lastMaterial->setTexture(MaterialParameter::NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
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
							_lastMaterial->setTexture(MaterialParameter::NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
					_lastMaterial->setTexture(MaterialParameter::OpacityMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
					log::warning("[OBJLoader] Illumination parameter ignored: %d", value);
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
				_lastMaterial->setFloat(MaterialParameter::Roughness, value);
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
					
					_lastMaterial = SceneMaterial::Pointer();
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
	uint32_t totalTriangles = 0;

	for (const auto& group : _groups)
	{
		for (const auto& face : group->faces)
		{
			ET_ASSERT(face.vertices.size() > 1);
			totalTriangles += static_cast<uint32_t>(face.vertices.size() - 2);
		}
	}
	
	uint32_t totalVertices = 3 * totalTriangles;
	
	bool hasNormals = _normals.size() > 0;
	bool hasTexCoords = _texCoords.size() > 0;
		
	VertexDeclaration decl(true, VertexAttributeUsage::Position, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, DataType::Vec3);

	if (hasTexCoords)
	{
		decl.push_back(VertexAttributeUsage::TexCoord0, DataType::Vec2);

		if ((_loadOptions & Option_CalculateTangents) == Option_CalculateTangents)
			decl.push_back(VertexAttributeUsage::Tangent, DataType::Vec3);
	}

	IndexArrayFormat fmt = (totalVertices > 65535) ? IndexArrayFormat::Format_32bit : IndexArrayFormat::Format_16bit;
	
	_indices = IndexArray::Pointer::create(fmt, totalVertices, PrimitiveType::Triangles);
	_indices->linearize(totalVertices);
	
	_vertexData = VertexStorage::Pointer::create(decl, totalVertices);

	auto pos = _vertexData->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	
	VertexDataAccessor<DataType::Vec3> nrm;
	if (_vertexData->hasAttributeWithType(VertexAttributeUsage::Normal, DataType::Vec3))
		nrm = _vertexData->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
	
	VertexDataAccessor<DataType::Vec2> tex;
	if (_vertexData->hasAttributeWithType(VertexAttributeUsage::TexCoord0, DataType::Vec2))
		tex = _vertexData->accessData<DataType::Vec2>(VertexAttributeUsage::TexCoord0, 0);
	
	uint32_t index = 0;
	
	auto PUSH_VERTEX = [this, &pos, &nrm, &tex, &index, hasTexCoords, hasNormals](const OBJVertex& vertex, const vec3& offset)
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
			nrm[index] = _normals[vertex[2]];
		}
		
		++index;
	};
	
	for (auto group : _groups)
	{
		size_t startIndex = index;
		
		vec3 center(0.0f);
		
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
		
		SceneMaterial::Pointer m;
		
		for (auto mat : _materials)
		{
			if (mat->name() == group->material)
			{
				m = mat;
				break;
			}
		}
		
		uint32_t startIndex_u32 = static_cast<uint32_t>(startIndex);
		uint32_t numIndexes_u32 = static_cast<uint32_t>(index - startIndex);
		_meshes.emplace_back(group->name, startIndex_u32, numIndexes_u32, m, center);
	}
	
	if (!hasNormals)
		primitives::calculateNormals(_vertexData, _indices, 0, _indices->primitivesCount());

	if (hasTexCoords && ((_loadOptions & Option_CalculateTangents) == Option_CalculateTangents))
		primitives::calculateTangents(_vertexData, _indices, 0, _indices->primitivesCount() & 0xffffffff);
}

s3d::ElementContainer::Pointer OBJLoader::generateVertexBuffers(s3d::Storage& storage)
{
	s3d::ElementContainer::Pointer result = s3d::ElementContainer::Pointer::create(inputFileName, nullptr);

	storage.flush();
	storage.addVertexStorage(_vertexData);
	storage.setIndexArray(_indices);
	
	for (auto m : _materials)
	{
		storage.addMaterial(m);
	}
	
	VertexArrayObject::Pointer vao = _rc->vertexBufferFactory().createVertexArrayObject("model-vao", _vertexData,
		BufferDrawType::Static, _indices, BufferDrawType::Static);

	uint32_t helperFlag = s3d::Flag_Helper * static_cast<uint32_t>((_loadOptions & Option_SupportMeshes) != 0);
	for (const auto& i : _meshes)
	{
		auto material = _materialProvider->materialWithName(i.material->name());
		
		auto rb = RenderBatch::Pointer::create(material, vao, translationMatrix(i.center), i.start, i.count);
		rb->setVertexStorage(_vertexData);
		rb->setIndexArray(_indices);
		
		s3d::Mesh::Pointer object = Mesh::Pointer::create(i.name, i.material, result.ptr());
		
		object->addRenderBatch(rb);
		object->setFlag(helperFlag);
	}

	return result;
}

void OBJLoader::threadFinished()
{
	ET_FAIL("TODO");
	/*
	 * TODO
	 *
	loaded.invoke(generateVertexBuffers(storage));
	// */
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
