/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/core/conversion.h>
#include <et/core/filesystem.h>
#include <et/rendering/base/primitives.h>
#include <et/rendering/base/material.h>
#include <et/scene3d/objloader.h>

using namespace et;
using namespace et::s3d;

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
				func(intToStr(std::numeric_limits<int32_t>::max()));
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
	static std::string ln;
	ln.reserve(128);
	ln.clear();

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
	static std::string ln;
	ln.reserve(128);
	ln.clear();

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
	static std::string ln;
	ln.reserve(128);
	ln.clear();

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

/*
 * OBJLoader
 */

OBJLoader::OBJLoader(const std::string& inFile, uint32_t options) :
	inputFileName(inFile.c_str()),
	inputFile(inputFileName.c_str()), _loadOptions(options)
{
	inputFilePath = getFilePath(inputFileName);
	
	if (inputFile.fail())
		log::info("Unable to open file %s", inputFileName.c_str());
}

OBJLoader::~OBJLoader()
{
	if (inputFile.is_open())
		inputFile.close();
	
	if (materialFile.is_open())
		materialFile.close();
}

float naive_atof(const char *p) 
{
	float r = 0.0;
	bool neg = false;
	if (*p == '-') {
		neg = true;
		++p;
	}
	while (*p >= '0' && *p <= '9') 
	{
		r = (r * 10.0f) + (*p - '0');
		++p;
	}
	if (*p == '.') 
	{
		float f = 0.0f;
		int32_t n = 0;
		++p;
		while (*p >= '0' && *p <= '9') 
		{
			f = (f * 10.0f) + (*p - '0');
			++p;
			++n;
		}
		r += f / std::pow(10.0f, n);
	}
	return neg ? -r : r;
}

void OBJLoader::load(ObjectsCache& cache)
{
	uint32_t lineNumber = 0;

	auto trimWhitespace = [](char* begin, char* end) -> char* {
		
		while (isWhitespaceChar(*begin))
			++begin;

		while ((end > begin) && (isWhitespaceChar(*(end - 1)))) {
			*(end - 1) = 0;
			--end;
		}

		return begin;
	};

	auto readFloats = [](char* line, char* end, uint32_t count, float* dst) {
		uint32_t floatsRead = 0;
		char* begin = line;
		while ((floatsRead < count) && (begin < end))
		{
			char* token = begin;
			while (!isWhitespaceChar(*token))
				++token;
			
			char tokenValue = *token;

			*token = 0;
			dst[floatsRead] = naive_atof(begin);
			*token = tokenValue;

			begin = token + 1;
			++floatsRead;
		}
	};

	const uint32_t BufferSize = 256 * 1024;
	char localBuffer[BufferSize + 3] = { };

	intptr_t readOffset = 0;
	intptr_t charactersRead = 0;
	do 
	{
		inputFile.read(localBuffer + readOffset, BufferSize - readOffset);
		charactersRead = inputFile.gcount();

		intptr_t currentBufferLength = readOffset + charactersRead;
		if (currentBufferLength > 0)
		{
			localBuffer[readOffset + charactersRead] = 0;
			readOffset = 0;

			char* begin = localBuffer;
			char* end = begin;
			while ((end - localBuffer) < currentBufferLength)
			{
				end = begin;
				while ((*end != 0) && (*end != '\n'))
					++end;

				if ((*end == '\n') || ((*end == 0) && inputFile.eof()))
				{
					*end = 0;
					{
						++lineNumber;

						char* line_begin = begin;
						char* local_end = end;
						char* local_begin = trimWhitespace(line_begin, local_end);
						if ((*local_begin == '#') || ((local_end - local_begin) == 0))
						{
							begin = end + 1;
							continue;
						}
						
						char* keyEnd = local_begin;
						while (!isWhitespaceChar(*keyEnd))
							++keyEnd;

						char* key = local_begin;
						*keyEnd = 0;

						char* local_value = trimWhitespace(keyEnd + 1, local_end);

						bool recognized = true;
						switch (*key)
						{
						case 'm':
						{
							if (strcmp(key, "mtllib") == 0)
							{
								loadMaterials(std::string(local_value, local_end - local_value), cache);
							}
							else
							{
								recognized = false;
							}
							break;
						}
						case 'g':
						{
							_groups.emplace_back(local_value, _sizeEstimate);
							break;
						}
						case 'u':
						{
							if (strcmp(key, "usemtl") == 0)
							{
								bool addGroup = _groups.empty() ||
									((strlen(_groups.back().material) > 0) && (strcmp(_groups.back().material, local_value) != 0));

								if (addGroup)
								{
									char buffer[OBJGroup::MaxGroupName] = {};
									sprintf(buffer, "group-%u-%s", static_cast<uint32_t>(_groups.size()), local_value);
									_groups.emplace_back(buffer, local_value, _sizeEstimate);
								}
								else
								{
									size_t stringSize = std::min(static_cast<size_t>(OBJGroup::MaxMaterialName), strlen(local_value));
									strncpy(_groups.back().material, local_value, stringSize);
								}
							}
							else
							{
								recognized = false;
							}
							break;
						}
						case 'v':
						{
							switch (*(key + 1))
							{
							case 0:
							{
								_vertices.emplace_back();
								readFloats(local_value, local_end, 3, _vertices.back().data());
								break;
							}
							case 'n':
							{
								_normals.emplace_back();
								readFloats(local_value, local_end, 3, _normals.back().data());
								break;
							}
							case 't':
							{
								_texCoords.emplace_back();
								readFloats(local_value, local_end, 2, _texCoords.back().data());
								break;
							}
							default:
								recognized = false;
							}
							break;
						}
						case 'f':
						{
							if (_groups.empty())
							{
								char buffer[OBJGroup::MaxGroupName] = {};
								sprintf("buffer", "group-%u", static_cast<uint32_t>(_groups.size()));
								_groups.emplace_back(buffer, _sizeEstimate);
							}

							_groups.back().faces.emplace_back();
							OBJFace& face = _groups.back().faces.back();

							while (local_value < local_end)
							{
								char* valueEnd = local_value;
								while ((valueEnd < local_end) && !isWhitespaceChar(*valueEnd))
									++valueEnd;

								char valueKey = *valueEnd;
								*valueEnd = 0;

								char* link = local_value;
								char* linkEnd = link;

								uint32_t linkIndex = 0;
								while (link < valueEnd)
								{
									char endValue = *linkEnd;
									if ((endValue == '/') || (endValue == 0))
									{
										*linkEnd = 0;

										int64_t linkValue = std::atoll(link);
										ET_ASSERT(linkValue > 0);
										ET_ASSERT(face.vertexLinksCount < OBJFace::MaxVertexLinks);

										face.vertexLinks[face.vertexLinksCount][linkIndex] = static_cast<uint32_t>(linkValue - 1);
										++linkIndex;

										*linkEnd = endValue;
										link = linkEnd + 1;
									}
									++linkEnd;
								}

								++face.vertexLinksCount;

								*valueEnd = valueKey;
								local_value = valueEnd + 1;
							}
							break;
						}
						case 's':
						case 'o':
							break;

						default:
							recognized = false;
							break;
						}

						if (!recognized)
						{
							log::warning("Unsupported entry `%s` in OBJ file at line %u", key, lineNumber);
						}
					}
					begin = end + 1;
				}
				else
				{
					readOffset = end - begin;
					memmove(localBuffer, begin, readOffset);
				}
			}
		}
	} while (charactersRead > 0);
}

void OBJLoader::loadData(ObjectsCache& cache)
{
	std::string line;
	uint32_t lineNumber = 0;
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
				loadMaterials(matName, cache);
			}
			else
			{
				std::cout << "Unresolved symbol: " << key << mtllib << std::endl;
			}
		}
		else if (key == 'o') // object, ignored for now
		{
			getLine(inputFile, line);
		}
		else if (key == 'g') // group
		{
			getLine(inputFile, line);
			trim(line);
			
			if (!_groups.empty())
			{
				_groups.back().faces.shrink_to_fit();
			}

            _groups.emplace_back(line, _sizeEstimate);
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
				
                if (_groups.empty())
                {
                    auto groupName = "group-" + intToStr(_lastGroupId++) + "-" + materialId;
                    _groups.emplace_back(groupName, materialId, _sizeEstimate);
                }
                else if ((strlen(_groups.back().material) == 0) || _groups.back().material == materialId)
                {
					size_t stringSize = std::min(static_cast<size_t>(OBJGroup::MaxMaterialName), materialId.size());
					strncpy(_groups.back().material, materialId.c_str(), stringSize);
                }
				getLine(inputFile, line);
			}
			else
			{
				std::cout << "Unresolved symbol " << key << usemtl << ". Current group: " << _groups.back().name << std::endl;
			}
		}
		else if (key == 's') // smoothing group
		{
            if (_groups.empty())
            {
                _groups.emplace_back("group-" + intToStr(_lastGroupId++), _sizeEstimate);
            }
			getLine(inputFile, line);
			_lastSmoothGroup = (line.compare("off") == 0) ? 0 : strToInt(line);
		}
		else if (key == 'f') // faces
		{
			OBJFace face;
			std::getline(inputFile, line);
			trim(line);
			
			static StringList faces;
			faces.reserve(5);
			faces.clear();

			splitAndWrite(line, ' ', [](const std::string& s) {
				faces.emplace_back(s);
			});
			ET_ASSERT((faces.size() > 2) && (faces.size() < OBJFace::MaxVertexLinks));

			for (auto inFace : faces)
			{
				static Vector<int32_t> indexes;
				indexes.reserve(3);
				indexes.clear();

				splitAndWrite(inFace, '/', [](const std::string& s)
					{ indexes.push_back(strToInt(s)); });

				uint32_t i = 0;
				face.vertexLinks[face.vertexLinksCount].fill(0);
				for (auto iValue : indexes)
				{
					if (iValue == std::numeric_limits<int32_t>::max())
					{
					}
					else if (iValue < 0)
					{
						uint32_t szValue = static_cast<uint32_t>(-iValue);
						if (i == 0)
						{
							ET_ASSERT(szValue <= _vertices.size());
							face.vertexLinks[face.vertexLinksCount][i] = static_cast<uint32_t>(_vertices.size() - szValue);
						}
						else if (i == 1)
						{
							ET_ASSERT(szValue <= _texCoords.size());
							face.vertexLinks[face.vertexLinksCount][i] = static_cast<uint32_t>(_texCoords.size() - szValue);
						}
						else if (i == 2)
						{
							ET_ASSERT(szValue <= _normals.size());
							face.vertexLinks[face.vertexLinksCount][i] = static_cast<uint32_t>(_normals.size() - szValue);
						}
					}
					else
					{
						face.vertexLinks[face.vertexLinksCount][i] = iValue - 1;
					}
					++i;
				}
				
				++face.vertexLinksCount;
			}
			
			if (_groups.empty())
			{
                _groups.emplace_back("group-" + intToStr(_lastGroupId++), _sizeEstimate);
			}
			
			_groups.back().faces.push_back(face);
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
				log::warning("Unknown line [%u] `%s`", lineNumber, line.c_str());
			}
		}
		else
		{
			getLine(inputFile, line);
			log::warning("Unknown line [%u] `%c%s`", lineNumber, key, line.c_str());
		}
	}
}

s3d::ElementContainer::Pointer OBJLoader::load(et::RenderInterface::Pointer ren, s3d::Storage& storage, ObjectsCache& cache)
{
	storage.flush();

	uint64_t fileSize = streamSize(inputFile);
	_sizeEstimate = std::max(1024llu, fileSize / 128);
	log::info("Loading OBJ, estimated array sizes: %llu", _sizeEstimate);

	_renderer = ren;
	_groups.reserve(128);
	_vertices.reserve(_sizeEstimate);
	_normals.reserve(_sizeEstimate);
	_texCoords.reserve(_sizeEstimate);
	
	// estimate for faces count
	_sizeEstimate = std::max(128llu, _sizeEstimate / 2048);
	log::info("Loading OBJ, estimated face array sizes: %llu", _sizeEstimate);

	auto t1 = queryCurrentTimeInMicroSeconds();
	load(cache);
	// loadData(cache);
	auto t2 = queryCurrentTimeInMicroSeconds();

	log::info("Loading time: %llu", t2 - t1);
	
	processLoadedData();

	s3d::ElementContainer::Pointer result = generateVertexBuffers(storage);
	loaded.invoke(result);

	sharedBlockAllocator().flushUnusedBlocks();

	return result;
}

void OBJLoader::loadMaterials(const std::string& fileName, ObjectsCache& cache)
{
	application().pushSearchPath(inputFilePath);
	std::string filePath = application().resolveFileName(fileName);
	application().popSearchPaths();

	if (_loadedMaterials.count(filePath) > 0)
		return;

	application().pushSearchPath(inputFilePath);
	_loadedMaterials.insert(filePath);
	
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
						_lastMaterial->setFloat(MaterialVariable::NormalScale, value);
						
						getLine(materialFile, line);
						std::string actualName = application().resolveFileName(line);
						_lastMaterial->setTexture(MaterialTexture::Normal, _renderer->loadTexture(actualName, cache));
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
					std::string actualName = application().resolveFileName(line);
					_lastMaterial->setTexture(MaterialTexture::Normal, _renderer->loadTexture(actualName, cache) );
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
					getLine(materialFile, line);
				}
				else if (next == 'd')
				{
					vec4 value(0.0f);
					materialFile >> value;
					_lastMaterial->setVector(MaterialVariable::DiffuseReflectance, value);
				} 
				else if (next == 's')
				{
					vec4 value(0.0f);
					materialFile >> value;
					_lastMaterial->setVector(MaterialVariable::SpecularReflectance, value);
				} 
				else if (next == 'e')
				{
					vec4 value(0.0f);
					materialFile >> value;
					_lastMaterial->setVector(MaterialVariable::EmissiveColor, value);
				} 
				else
				{
					getLine(materialFile, line);
					std::cout << "Unresolved (unsupported) material parameter:\n" << key << next << line << std::endl;
				}
			}
		}
		else if (key == 'p')
		{
			char next = 0;
			materialFile >> next;
			next = static_cast<char>(tolower(next));

			if (next == 'r')
			{
				float value = 0.0f;
				materialFile >> value;
				_lastMaterial->setFloat(MaterialVariable::RoughnessScale, value);
			}
			else if (next == 'm')
			{
				float value = 0.0f;
				materialFile >> value;
				_lastMaterial->setFloat(MaterialVariable::MetallnessScale, value);
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
				_lastMaterial->setFloat(MaterialVariable::OpacityScale, clamp(1.0f - value, 0.0f, 1.0f));
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
						std::string actualName = application().resolveFileName(line);
						_lastMaterial->setTexture(MaterialTexture::BaseColor, _renderer->loadTexture(actualName, cache) );
					}
					else if (subId == 'a')
					{
						getLine(materialFile, line);
					}
					else if (subId == 's')
					{
						getLine(materialFile, line);
						std::string actualName = application().resolveFileName(line);
						log::warning("Reflectance texture ingored (map_Ks) in material");
						// _lastMaterial->setTexture(MaterialTexture::Reflectance, _renderer->loadTexture(actualName, cache) );
					}
					else if (subId == 'e')
					{
						getLine(materialFile, line);
						std::string actualName = application().resolveFileName(line);
						_lastMaterial->setTexture(MaterialTexture::EmissiveColor, _renderer->loadTexture(actualName, cache) );
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
								_lastMaterial->setFloat(MaterialVariable::NormalScale, value);
								
								getLine(materialFile, line);
								std::string actualName = application().resolveFileName(line);
								_lastMaterial->setTexture(MaterialTexture::Normal, _renderer->loadTexture(actualName, cache));
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
							std::string actualName = application().resolveFileName(line);
							_lastMaterial->setTexture(MaterialTexture::Normal, _renderer->loadTexture(actualName, cache) );
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
					std::string actualName = application().resolveFileName(line);
					log::warning("Opacity texture (map_d) ignored in material - move opacity to baseColor's alpha");
					// _lastMaterial->setTexture(MaterialTexture::Opacity, _renderer->loadTexture(actualName, cache) );
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
				_lastMaterial->setFloat(MaterialVariable::SpecularExponent, value);
			}
			else if (next == 'i')
			{
				float value = 0.0f;
				materialFile >> value;
				_lastMaterial->setFloat(MaterialVariable::IndexOfRefraction, value);
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

					Material::Pointer mtl = _renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::Microfacet);
					_lastMaterial = mtl->instance();
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

	for (const OBJGroup& group : _groups)
	{
		for (const OBJFace& face : group.faces)
		{
			ET_ASSERT(face.vertexLinksCount > 1);
			totalTriangles += static_cast<uint32_t>(face.vertexLinksCount - 2);
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
	
	for (const OBJGroup& group : _groups)
	{
		uint32_t startIndex = index;
		
		vec3 center(0.0f);
		
		if (_loadOptions & Option_CalculateTransforms)
		{
			uint32_t vertexCount = 0;
			
			for (const OBJFace& face : group.faces)
			{
				uint32_t numTriangles = face.vertexLinksCount - 2;
				for (uint32_t i = 1; i <= numTriangles; ++i)
				{
					center += _vertices[face.vertexLinks[0][0]];
					center += _vertices[face.vertexLinks[i][0]];
					center += _vertices[face.vertexLinks[i+1][0]];
					vertexCount += 3;
				}
			}
			
			if (vertexCount > 0.0f)
				center /= static_cast<float>(vertexCount);
		}
		
		for (const OBJFace& face : group.faces)
		{
			uint32_t numTriangles = face.vertexLinksCount - 2;
			for (uint32_t i = 1; i <= numTriangles; ++i)
			{
				pos[index+0] = _vertices[face.vertexLinks[0][0]] - center;
				pos[index+1] = _vertices[face.vertexLinks[i][0]] - center;
				pos[index+2] = _vertices[face.vertexLinks[i+1][0]] - center;
				if (hasTexCoords)
				{
					tex[index+0] = _texCoords[face.vertexLinks[0][1]];
					tex[index+1] = _texCoords[face.vertexLinks[i][1]];
					tex[index+2] = _texCoords[face.vertexLinks[i+1][1]];
				}
				if (hasNormals)
				{
					nrm[index+0] = _normals[face.vertexLinks[0][2]];
					nrm[index+1] = _normals[face.vertexLinks[i][2]];
					nrm[index+2] = _normals[face.vertexLinks[i+1][2]];
				}
				index += 3;
			}
		}

		MaterialInstance::Pointer m;
		for (const MaterialInstancePointer& mat : _materials)
		{
			if (mat->name() == group.material)
			{
				m = mat;
				break;
			}
		}

		if (m.invalid())
		{
			log::error("Unable to find material `%s`", group.material);
			Material::Pointer microfacet = _renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::Microfacet);
			m = microfacet->instance();
			m->setName("missing_material");
		}
		
		uint32_t startIndex_u32 = static_cast<uint32_t>(startIndex);
		uint32_t numIndexes_u32 = static_cast<uint32_t>(index - startIndex);
		_meshes.emplace_back(group.name, startIndex_u32, numIndexes_u32, m, center);
	}
	
	if (!hasNormals)
		primitives::calculateNormals(_vertexData, _indices, 0, _indices->primitivesCount());

	if ((_loadOptions & Option_CalculateTangents) == Option_CalculateTangents)
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
	
	Buffer::Pointer vb = _renderer->createVertexBuffer("model-vb", _vertexData, Buffer::Location::Device);
	Buffer::Pointer ib = _renderer->createIndexBuffer("model-ib", _indices, Buffer::Location::Device);
	
	VertexStream::Pointer vao = VertexStream::Pointer::create();
	vao->setVertexBuffer(vb, _vertexData->declaration());
	vao->setIndexBuffer(ib, _indices->format(), _indices->primitiveType());

	vec3 minExtent(+std::numeric_limits<float>::max());
	vec3 maxExtent(-std::numeric_limits<float>::max());
	for (auto& i : _meshes)
	{
		s3d::Mesh::Pointer mesh = Mesh::Pointer::create(i.name, result.pointer());
		mesh->setTranslation(i.center);

		auto rb = RenderBatch::Pointer::create(i.material, vao, i.start, i.count);
		rb->setVertexStorage(_vertexData);
		rb->setIndexArray(_indices);
		mesh->addRenderBatch(rb);
		mesh->calculateSupportData();
		maxExtent = maxv(maxExtent, mesh->tranformedBoundingBox().maxVertex());
		minExtent = minv(minExtent, mesh->tranformedBoundingBox().minVertex());
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
	trim(line);
}
