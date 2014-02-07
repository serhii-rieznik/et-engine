/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/imaging/hdrloader.h>

using namespace et;

const std::string kRadianceHeader = "#?RADIANCE";
const std::string kRadianceFormatEntry = "FORMAT=";
const std::string kRadiance32Bit_RLE_RGBE = "32-BIT_RLE_RGBE";

typedef unsigned char RGBEEntry[4];

#define CONVERT_RGBE	1

void readScanline(std::istream&, int width, RGBEEntry* data);
void readScanlineLegacy(std::istream&, int width, RGBEEntry* data);
vec4 rgbeToFloat(RGBEEntry*);

void et::hdr::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	std::string line;
	std::getline(source, line);
	
	if (line != kRadianceHeader)
		return;

	std::getline(source, line);
	while (line.empty() || (line.find('#') == 0))
		std::getline(source, line);
		
	uppercase(line);
	
	if (line.find(kRadianceFormatEntry) != 0)
		return;
	
	std::string format = line.substr(kRadianceFormatEntry.size());
	if (format != kRadiance32Bit_RLE_RGBE)
		return;
	
	std::getline(source, line);
	
	while (line.empty() || (line.find('#') == 0))
		std::getline(source, line);
	
	uppercase(line);
	line = removeWhitespace(line);
	
	size_t xpos = line.find('X');
	size_t ypos = line.find('Y');
	if ((xpos == std::string::npos) || (ypos == std::string::npos))
		return;
	
	std::string ws;
	std::string hs;
	
	if (xpos < ypos)
	{
		ws = line.substr(xpos + 1, ypos - xpos - 2);
		hs = line.substr(ypos + 1);
	}
	else
	{
		hs = line.substr(ypos + 1, xpos - ypos - 2);
		ws = line.substr(xpos + 1);
	}
	
	log::info("W: %s, H: %s", ws.c_str(), hs.c_str());
	
	desc.size.x = strToInt(ws);
	desc.size.y = strToInt(hs);
	
	desc.target = GL_TEXTURE_2D;
	desc.format = GL_RGBA;
	
#if (CONVERT_RGBE)
	desc.internalformat = GL_RGBA32F;
	desc.type = GL_FLOAT;
	desc.bitsPerPixel = 128;
#else
	desc.internalformat = GL_RGBA;
	desc.type = GL_UNSIGNED_BYTE;
	desc.bitsPerPixel = 32;
#endif
	
	desc.mipMapCount = 1;
	desc.compressed = 0;
	desc.channels = 4;
	desc.layersCount = 1;
}

void et::hdr::loadFromStream(std::istream& source, TextureDescription& desc)
{
	loadInfoFromStream(source, desc);
	
#if (CONVERT_RGBE)
	
	size_t rowSize = desc.size.x * 4;
	BinaryDataStorage rgbeData(desc.size.y * rowSize, 0);
	for (int y = 0; y < desc.size.y; ++y)
		readScanline(source, desc.size.x, reinterpret_cast<RGBEEntry*>(rgbeData.element_ptr((desc.size.y - 1 - y) * rowSize)));
	
	desc.data = BinaryDataStorage(desc.size.square() * desc.bitsPerPixel / 8, 0);
	vec4* floats = reinterpret_cast<vec4*>(desc.data.binary());
	for (size_t i = 0; i < desc.size.square(); ++i)
		*floats++ = rgbeToFloat(reinterpret_cast<RGBEEntry*>(rgbeData.element_ptr(i * 4)));
	
#else
	
	size_t rowSize = desc.size.x * 4;
	desc.data.resize(desc.size.y * rowSize);
	for (int y = 0; y < desc.size.y; ++y)
		readScanline(source, desc.size.x, reinterpret_cast<RGBEEntry*>(desc.data.element_ptr((desc.size.y - 1 - y) * rowSize)));
	
#endif
}

void et::hdr::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void et::hdr::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}

/*
 * Internal stuff
 */

#define R			0
#define G			1
#define B			2
#define E			3

#define  MINELEN	8				// minimum scanline length for encoding
#define  MAXELEN	0x7fff			// maximum scanline length for encoding

void readScanline(std::istream& file, int width, RGBEEntry* scanline)
{
	if ((width < MINELEN) || (width > MAXELEN))
	{
		return readScanlineLegacy(file, width, scanline);
		return;
	}
	
	int i = file.get();
	if (i != 2)
	{
		file.seekg(-1, std::ios_base::cur);
		readScanlineLegacy(file, width, scanline);
		return;
	}
	scanline[0][G] = file.get();
	scanline[0][B] = file.get();
	
	i = file.get();
	
	if ((scanline[0][G] != 2) || (scanline[0][B] & 128))
	{
		scanline[0][R] = 2;
		scanline[0][E] = i;
		return readScanlineLegacy(file, width - 1, scanline + 1);
	}
	
	for (i = 0; i < 4; i++)
	{
	    for (int j = 0; j < width;)
		{
			unsigned char code = file.get();
			if (code > 128)
			{
			    code &= 127;
			    unsigned char val = file.get();
			    while (code--)
					scanline[j++][i] = val;
			}
			else
			{
			    while (code--)
					scanline[j++][i] = file.get();
			}
		}
    }
}

void readScanlineLegacy(std::istream&, int width, RGBEEntry* data)
{
	ET_FAIL("Not implemented.");
}

float convertComponent(int expo, int val)
{
	float v = static_cast<float>(val) / 256.0f;
	float d = std::pow(2.0f, static_cast<float>(expo));
	return v * d;
}

vec4 rgbeToFloat(RGBEEntry* scan)
{
	vec4 result;
	
	int expo = scan[0][E] - 128;
	result.x = convertComponent(expo, scan[0][R]);
	result.y = convertComponent(expo, scan[0][G]);
	result.z = convertComponent(expo, scan[0][B]);
	result.w = 1.0;
	return result;
}
