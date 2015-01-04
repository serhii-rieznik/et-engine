/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/hdrloader.h>

using namespace et;

const std::string kRadianceHeader = "#?RADIANCE";
const std::string kRadianceFormatEntry = "FORMAT=";
const std::string kRadiance32Bit_RLE_RGBE = "32-BIT_RLE_RGBE";

static bool shouldConvertRGBEToFloat = true;

void et::hdr::setShouldConvertRGBEToFloat(bool value)
{
	shouldConvertRGBEToFloat = value;
}

unsigned char* readScanline(unsigned char*, int, vec4ub*);
vec4 rgbeToFloat(const vec4ub&);

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
	
	desc.size.x = strToInt(ws);
	desc.size.y = strToInt(hs);
	
	desc.target = TextureTarget::Texture_2D;
	desc.format = TextureFormat::RGB;
	
	if (shouldConvertRGBEToFloat)
	{
		desc.internalformat = TextureFormat::RGBA32F;
		desc.type = DataType::Float;
		desc.bitsPerPixel = 128;
	}
	else
	{
		desc.internalformat = TextureFormat::RGBA;
		desc.type = DataType::UnsignedChar;
		desc.bitsPerPixel = 32;
	}
	
	desc.channels = 4;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.compressed = 0;
}

void et::hdr::loadFromStream(std::istream& source, TextureDescription& desc)
{
	loadInfoFromStream(source, desc);

	if ((desc.size.x < 8) || (desc.size.x > 0x7fff))
		ET_FAIL("Unsupported HDR format.");

	auto sourcePos = source.tellg();

	size_t rowSize = desc.size.x * 4;
	size_t maxDataSize = 8 * desc.size.square();
	BinaryDataStorage inData(maxDataSize, 0);
	source.read(inData.binary(), maxDataSize);
	auto ptr = inData.begin();

	if (shouldConvertRGBEToFloat)
	{
		BinaryDataStorage rgbeData(desc.size.y * rowSize, 0);

		for (int y = 0; y < desc.size.y; ++y)
			ptr = readScanline(ptr, desc.size.x, reinterpret_cast<vec4ub*>(rgbeData.element_ptr((desc.size.y - 1 - y) * rowSize)));
		
		desc.data.resize(desc.size.square() * desc.bitsPerPixel / 8);
		
		vec4* floats = reinterpret_cast<vec4*>(desc.data.binary());
		vec4ub* rgbe = reinterpret_cast<vec4ub*>(rgbeData.begin());
		for (int i = 0; i < desc.size.square(); ++i)
			*floats++ = rgbeToFloat(*rgbe++);
		
	}
	else
	{
		desc.data.resize(desc.size.y * rowSize);
		for (int y = 0; y < desc.size.y; ++y)
			ptr = readScanline(ptr, desc.size.x, reinterpret_cast<vec4ub*>(desc.data.element_ptr((desc.size.y - 1 - y) * rowSize)));
	}
	
	
	decltype(sourcePos) dataSize = ptr - inData.begin();
	source.seekg(sourcePos + dataSize, std::ios::beg);
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

unsigned char* readScanline(unsigned char* ptr, int width, vec4ub* scanline)
{
	if (*ptr++ == 2)
	{
		scanline->y = *ptr++;
		scanline->z = *ptr++;

		++ptr;

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < width;)
			{
				unsigned char code = *ptr++;
				if (code > 128)
				{
					code &= 127;
					unsigned char val = *ptr++;
					while (code--)
						scanline[j++][i] = val;
				}
				else
				{
					while (code--)
						scanline[j++][i] = *ptr++;
				}
			}
		}
	}
	else
	{
		ET_FAIL("Legacy scanlines are not supported");
	}

	return ptr;
}

inline float convertComponent(char expo, unsigned char val)
{
	return (expo > 0) ? static_cast<float>(val * (1 << expo)) : 
		static_cast<float>(val) / static_cast<float>(1 << -expo);
}

vec4 rgbeToFloat(const vec4ub& data)
{
	char expo = data.w - 128;
	return vec4(convertComponent(expo, data.x), convertComponent(expo, data.y),
		convertComponent(expo, data.z), 256.0f) / 256.0f;
}
