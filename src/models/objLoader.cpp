/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <sstream>
#include <iostream>

#include <et/core/conversion.h>
#include <et/core/filesystem.h>
#include <et/models/objloader.h>
#include <et/apiobjects/vertexbuffer.h>
#include <et/primitives/primitives.h>
#include <et/app/application.h>

using namespace et;
using namespace et::s3d;
using namespace et::obj;

OBJLoaderThread::OBJLoaderThread(OBJLoader* owner, ObjectsCache& cache) : Thread(false), 
	_owner(owner), _cache(cache)
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
	inputFileName(application().environment().findFile(inFile).c_str()), 
	inputFile(inputFileName.c_str()), lastGroup(0)
{
	inputFilePath = getFilePath(inputFileName);
	canConvert = !inputFile.fail();
	
	application().environment().addSearchPath(inputFilePath);
	
	if (!canConvert)
		std::cout << "Can't open file for input: " << inFile << std::endl;
}

OBJLoader::~OBJLoader()
{
	for (GroupList::iterator gi = groups.begin(), ge = groups.end(); gi != ge; ++gi)
		delete (*gi);
	
	if (inputFile.is_open())
		inputFile.close();
	
	if (materialFile.is_open())
		materialFile.close();
}

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
			lastGroup = new OBJGroup;
			groups.push_back(lastGroup);
			
			getLine(inputFile, line);
			
			std::string lineStr(line);
			trim(lineStr);
			
			lastGroup->name = lineStr;
		}
		else if (key == 'u') // group's material
		{
			std::string usemtl;
			inputFile >> usemtl;
			lowercase(usemtl);
			
			if (usemtl.compare("semtl") == 0) 
			{
				if (lastGroup == 0)
				{
					lastGroup = new OBJGroup();
					groups.push_back(lastGroup);
					lastGroup->name = "group" + intToStr(lastGroupId_);
					++lastGroupId_;
					std::cout << "Group created: " << lastGroupId_;
				}
				inputFile >> lastGroup->material;
			}
			else
			{
				std::cout << "Unresolved symbol " << key << usemtl << ". Current group: " << lastGroup->name << std::endl;
			}
		}
		else if (key == 's') // smoothing group
		{
			if (lastGroup == 0)
			{
				lastGroup = new OBJGroup();
				groups.push_back(lastGroup);
				lastGroup->name = "group" + intToStr(lastGroupId_);
				++lastGroupId_;
				std::cout << "Group created: " << lastGroupId_;
			}
			getLine(inputFile, line);
			lastSmoothGroup = (line.compare("off") == 0) ? 0 : strToInt(line);
		}
		else if (key == 'f') // faces
		{
			OBJVertex vertex;
			OBJFace face;
			
			inputFile >> vertex;
			face.smoothingGroupIndex = lastSmoothGroup;
			face.vertices.push_back(vertex);
			
			int numF = 1;
			char delim = static_cast<char>(inputFile.peek());
			while ((numF < 3) && (delim == ' '))
			{
				inputFile >> vertex;
				face.vertices.push_back(vertex);
				delim = static_cast<char>(inputFile.peek());
				++numF;
			}
			getLine(inputFile, line);
			
			if (!lastGroup)
				lastGroup = new OBJGroup;
			
			lastGroup->faces.push_back(face);
		}
		else if (key == 'v')
		{
			char subKey = static_cast<char>(inputFile.peek());
			
			if (subKey == 't')
			{
				vec2 vertex;
				inputFile >> subKey >> vertex;
				texCoords.push_back(vertex);
				
				if (!((inputFile.peek() == '\r') || (inputFile.peek() == '\n')) )
					getLine(inputFile, line);
				
			}
			else if (subKey == 'n')
			{
				vec3 vertex;
				inputFile >> subKey >> vertex;
				normals.push_back(vertex);
			}
			else if (subKey == 32)
			{
				vec3 vertex;
				inputFile >> vertex;
				vertices.push_back(vertex);
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
/*	
	cout << "Conversion result:\n";
	cout << vertices.size() << " vertices\n";
	cout << texCoords.size() << " texture coords\n";
	cout << normals.size() << " normals\n";
	cout << groups.size() << " groups:\n";
	
	for (GroupList::iterator i = groups.begin(), e = groups.end(); i != e; ++i)
		cout << "\t" << (*i)->name << ", with material " << (*i)->material << " and " << (*i)->faces.size() << " faces.\n";
*/	

}

s3d::ElementContainer::Pointer OBJLoader::load(ObjectsCache& cache)
{
	loadData(false, cache);
	processLoadedData();

	s3d::ElementContainer::Pointer result = generateVertexBuffers();
	loaded.invoke(result);
	return result;
}

void OBJLoader::loadAsync(ObjectsCache& cache)
{
	_thread = new OBJLoaderThread(this, cache);
}

void OBJLoader::loadMaterials(const std::string& fileName, bool async, ObjectsCache& cache)
{
	std::string filePath = application().environment().findFile(fileName);
	if (!fileExists(filePath))
		filePath = application().environment().findFile(inputFilePath + fileName);
	
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
						float value;
						materialFile >> value;
						lastMaterial->setFloat(MaterialParameter_BumpFactor, value);
						
						getLine(materialFile, line);
						lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
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
					lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
			if (lastMaterial == 0)
			{
				getLine(materialFile, line);
			}
			else
			{
				char next;
				materialFile >> next;
				next = static_cast<char>(tolower(next));
				
				if (next == 'a')
				{
					vec4 value;
					materialFile >> value;
					lastMaterial->setVector(MaterialParameter_AmbientColor, value);
				} 
				else if (next == 'd')
				{
					vec4 value;
					materialFile >> value;
					lastMaterial->setVector(MaterialParameter_DiffuseColor, value);
				} 
				else if (next == 's')
				{
					vec4 value;
					materialFile >> value;
					lastMaterial->setVector(MaterialParameter_SpecularColor, value);
				} 
				else if (next == 'e')
				{
					vec4 value;
					materialFile >> value;
					lastMaterial->setVector(MaterialParameter_EmissiveColor, value);
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
			char next;
			materialFile >> next;
			next = static_cast<char>(tolower(next));
			
			if (next == 'r')
			{
				float value;
				materialFile >> value;
				lastMaterial->setFloat(MaterialParameter_Transparency, value);
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
			char map[4] = {};
			materialFile >> map[0] >> map[1] >> map[2];
			
			if (strcmp(map, "ap_") == 0)
			{
				char mapId;
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
						lastMaterial->setTexture( MaterialParameter_DiffuseMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'a')
					{
						getLine(materialFile, line);
						lastMaterial->setTexture( MaterialParameter_AmbientMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 's')
					{
						getLine(materialFile, line);
						lastMaterial->setTexture( MaterialParameter_SpecularMap, _rc->textureFactory().loadTexture(line, cache, async) );
					}
					else if (subId == 'e')
					{
						getLine(materialFile, line);
						lastMaterial->setTexture( MaterialParameter_EmissiveMap, _rc->textureFactory().loadTexture(line, cache, async) );
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
								lastMaterial->setFloat(MaterialParameter_BumpFactor, value);
								
								getLine(materialFile, line);
								lastMaterial->setTexture(MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async));
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
							lastMaterial->setTexture( MaterialParameter_NormalMap, _rc->textureFactory().loadTexture(line, cache, async) );
						}
					}
					else
					{
						std::cout << "Unknown map type: " << mapId << bump << std::endl;
					}
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
			if (lastMaterial == 0)
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
					int value;
					materialFile >> value;
					lastMaterial->setInt(MaterialParameter_ShadingModel, value);
				}
				else
				{
					std::cout << "Unresolved symbol: " << key << illum << std::endl;
				}
			}
		}
		else if (key == 'n')
		{
			char next;
			materialFile >> next;
			next = static_cast<char>(tolower(next));
			
			if (next == 's')
			{
				float value;
				materialFile >> value;
				lastMaterial->setFloat(MaterialParameter_Roughness, value);
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
					lastMaterial = Material();
					materials.push_back(lastMaterial);
					std::string name;
					materialFile >> name;
					lastMaterial->setName(name);
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
	
	std::cout << "Materials loaded.\n";
	materialFile.close();
}

void OBJLoader::processLoadedData()
{
	size_t totalTriangles = 0;
	for (size_t g = 0; g < groups.size(); ++g)
		totalTriangles += groups[g]->faces.size();
	
	size_t totalVertices = totalTriangles * 3;
	
	bool hasNormals = normals.size() > 0;
	bool hasTexCoords = texCoords.size() > 0;
		
	VertexDeclaration decl(true);
	decl.push_back(Usage_Position, Type_Vec3);
		
	if (hasNormals)
		decl.push_back(Usage_Normal, Type_Vec3);

	if (hasTexCoords)
		decl.push_back(Usage_TexCoord0, Type_Vec2);
		
	IndexArrayFormat fmt = IndexArrayFormat_8bit;
	if (totalVertices > 255)
		fmt = IndexArrayFormat_16bit;
	if (totalVertices > 65535)
		fmt = IndexArrayFormat_32bit;
		
	_indices = IndexArray::Pointer(new IndexArray(fmt, totalVertices, PrimitiveType_Triangles));
	_indices->linearize(totalVertices);

	_vertexData.reset(new VertexArray(decl, totalVertices));
	VertexDataChunk pos_c = _vertexData->chunk(Usage_Position);
	VertexDataChunk norm_c = _vertexData->chunk(Usage_Normal);
	VertexDataChunk tex_c = _vertexData->chunk(Usage_TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(0) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(0) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(0) : RawDataAcessor<vec2>();
		
	size_t index = 0;
	for (GroupList::iterator gi = groups.begin(), ge = groups.end(); gi != ge; ++gi)
	{
		IndexType startIndex = static_cast<IndexType>(index);
		for (FaceList::iterator fi = (*gi)->faces.begin(), fe = (*gi)->faces.end(); fi != fe; ++fi)
		{
			for (VertexList::iterator vi = fi->vertices.begin(), ve = fi->vertices.end(); vi != ve; ++vi)
			{
				size_t v_id = vi->v[0];
				size_t t_id = vi->v[1];
				size_t n_id = vi->v[2];
					
				pos[index] = vertices[v_id];
					
				if (hasNormals)
					norm[index] = normals[n_id];
					
				if (hasTexCoords)
					tex[index] = texCoords[t_id];

				++index;
			}
		}
			
		Material m;
		for (Material::List::iterator mi = materials.begin(), me = materials.end(); mi != me; ++mi)
		{
			if ((*mi)->name() == (*gi)->material)
			{
				m = *mi;
				break;
			}
		}
			
		_meshes.push_back(OBJMeshIndexBounds((*gi)->name, startIndex, index - startIndex, m));
	}
}

s3d::ElementContainer::Pointer OBJLoader::generateVertexBuffers()
{
	s3d::ElementContainer::Pointer result(new ElementContainer(inputFileName, 0));

	VertexArrayObject vao = _rc->vertexBufferFactory().createVertexArrayObject("model-vao");
	vao->setBuffers(_rc->vertexBufferFactory().createVertexBuffer("model-vb", _vertexData, BufferDrawType_Static),
					_rc->vertexBufferFactory().createIndexBuffer("model-ib", _indices, BufferDrawType_Static));

	for (OBJMeshIndexBoundsList::iterator i = _meshes.begin(), e = _meshes.end(); i != e; ++i)
		new Mesh(i->name, vao, i->material, i->start, i->count, result.ptr());

	return result;
}

void OBJLoader::threadFinished()
{
	loaded.invoke(generateVertexBuffers());
}