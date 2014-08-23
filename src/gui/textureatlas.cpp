/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <sstream>
#include <et/core/et.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/textureloader.h>
#include <et/gui/textureatlas.h>

using namespace et;
using namespace et::gui;

static const Image _emptyImage;

rect parseRectString(std::string& s);

TextureAtlas::TextureAtlas() : _loaded(false)
{
}

TextureAtlas::TextureAtlas(RenderContext* rc, const std::string& filename, ObjectsCache& cache) :
	_loaded(false)
{
	loadFromFile(rc, filename, cache);
}

void TextureAtlas::loadFromFile(RenderContext* rc, const std::string& filename, ObjectsCache& cache)
{
	std::string resolvedFileName = application().resolveFileName(filename);
	
	InputStream descFile(resolvedFileName, StreamMode_Text);
	if (descFile.invalid()) return;

	std::string filePath = getFilePath(resolvedFileName);
	int lineNumber = 1;

	while (!descFile.stream().eof())
	{
		std::string token;
		std::string line;

		descFile.stream() >> token;
		std::getline(descFile.stream(), line);

		if (token == "texture:")
		{
			std::string textureId = trim(line);
			std::string textureName = application().resolveFileName(textureId);
			
			if (!fileExists(textureName))
				textureName = application().resolveFileName(filePath + textureId);
			
			_textures[textureId] = rc->textureFactory().loadTexture(textureName, cache);
			_textures[textureId]->setWrap(rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
		}
		else if (token == "image:")
		{
			trim(line);
			if ((*line.begin() == '{') && (*line.rbegin() == '}'))
			{
				line = line.substr(1, line.length() - 2);
				trim(line);
				std::istringstream parser(line);

				std::string imageName;
				std::string textureName;
				rect sourceRect;
				rect contentOffset;

				while (!parser.eof())
				{
					std::string aToken;
					parser >> aToken;
					if (aToken == "name:")
					{
						parser >> imageName;
						if (*imageName.begin() == '"')
							imageName.erase(0, 1);
						if (*imageName.rbegin() == '"')
							imageName.erase(imageName.length() - 1);
					}
					else if (aToken == "texture:")
					{
						parser >> textureName;
						if (*textureName.begin() == '"')
							textureName.erase(0, 1);
						if (*textureName.rbegin() == '"')
							textureName.erase(textureName.length() - 1);
					}
					else if (aToken == "rect:")
					{
						std::string sRect;
						parser >> sRect;
						sourceRect = parseRectString(sRect);
					}
					else if (aToken == "offset:")
					{
						std::string offset;
						parser >> offset;
						contentOffset = parseRectString(offset);
					}
					else
					{
						log::warning("Unknown token at line %d : %s", lineNumber, aToken.c_str());
					}
				}

				ImageDescriptor desc(sourceRect.origin(), sourceRect.size());

				desc.contentOffset = ContentOffset(contentOffset[0], contentOffset[1],
					contentOffset[2], contentOffset[3]);

				_images[imageName] = Image(_textures[textureName], desc);
			}
			else 
			{
				log::warning("Unable to parse image token at line %d : %s", lineNumber, line.c_str());
			}
		}
		else 
		{
			if (token.length() && line.length())
				log::warning("Unknown token at line %d : %s", lineNumber, token.c_str());
		}

		++lineNumber;
	}

	_loaded = true;
}

const gui::Image& TextureAtlas::image(const std::string& key) const
{
	if (key.length() == 0) return _emptyImage;
	
	auto i = _images.find(key);
	
	if (i == _images.end())
		return _emptyImage;
	
	return i->second;
}

ImageList TextureAtlas::imagesForTexture(Texture t) const
{
	ImageList result;
	
	for (auto& i : _images)
	{
		if (i.second.texture == t)
			result.push_back(i.second);
	}
			
	return result;
}

void TextureAtlas::unload()
{
	_images.clear();
	_textures.clear();
	_loaded = false;
}

Texture TextureAtlas::firstTexture() const
{
	return _textures.size() ? _textures.begin()->second : Texture();
}

/*
 * Helper funcitons
 */
rect parseRectString(std::string& s)
{
	rect result;
	int values[4] = { };
	int vIndex = 0;

	if (*s.begin() == '"')
		s.erase(0, 1);

	if (*s.rbegin() == '"')
		s.erase(s.length() - 1);

	while (s.length())
	{
		std::string::size_type dpos = s.find_first_of(";");
		if (dpos == std::string::npos)
		{
			values[vIndex++] = strToInt(s);
			break;
		}
		else
		{
			std::string value = s.substr(0, dpos);
			values[vIndex++] = strToInt(value);
			s.erase(0, dpos+1);
		}
	}

	result.left = static_cast<float>(values[0]);
	result.top = static_cast<float>(values[1]);
	result.width = static_cast<float>(values[2]);
	result.height = static_cast<float>(values[3]);

	return result;
}
