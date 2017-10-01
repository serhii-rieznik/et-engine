/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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

void et::hdr::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	std::string line;
	std::getline(source, line);
	
	if (line != kRadianceHeader)
	{
		log::error("Failed to load HDR image: invalid header");
		return;
	}

	std::getline(source, line);
	while (line.empty() || (line.find('#') == 0))
		std::getline(source, line);
		
	uppercase(line);
	
	if (line.find(kRadianceFormatEntry) != 0)
	{
		log::error("Failed to load HDR image: can't find format in header");
		return;
	}
	
	std::string format = line.substr(kRadianceFormatEntry.size());
	if (format != kRadiance32Bit_RLE_RGBE)
	{
		log::error("Failed to load HDR image: invalid format (%s)", format.c_str());
		return;
	}

	std::getline(source, line);
	uppercase(line);

	while (line.empty() || (line.find('#') == 0) || (line.find("EXPOSURE") == 0) || (line.find("GAMMA") == 0))
	{
		std::getline(source, line);
		uppercase(line);
	}
	
	line = removeWhitespace(line);
	
	size_t xpos = line.find('X');
	size_t ypos = line.find('Y');
	if ((xpos == std::string::npos) || (ypos == std::string::npos))
	{
		log::error("Failed to load HDR image: can't find dimensions in header (last checked line: %s)", line.c_str());
		return;
	}

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
	desc.format = shouldConvertRGBEToFloat ? TextureFormat::RGBA32F : TextureFormat::RGBA8;	
	desc.levelCount = 1;
	desc.layerCount = 1;
}

void et::hdr::loadFromStream(std::istream& source, TextureDescription& desc)
{
	loadInfoFromStream(source, desc);

	if ((desc.size.x < 8) || (desc.size.x > 0x7fff))
	{
		log::error("Failed to load HDR image");
		return;
	}

	auto sourcePos = source.tellg();

    uint64_t square = desc.size.square();
    uint64_t maxDataSize = square * bitsPerPixelForTextureFormat(desc.format) / 8;
    
    uint64_t rowSize = desc.size.x * 4;
	BinaryDataStorage inData(maxDataSize, 0);
	source.read(inData.binary(), maxDataSize);
	uint8_t* ptr = inData.begin();

	if (shouldConvertRGBEToFloat)
	{
		desc.data.resize(desc.size.square() * sizeof(vec4));

		DataStorage<vec4ub> rgbeData(desc.size.square(), 0);
		DataStorage<vec4> floatDataWrapper(reinterpret_cast<vec4*>(desc.data.data()), desc.data.size());

		for (int32_t y = 0; y < desc.size.y; ++y)
		{
			auto rowPtr = rgbeData.binary() + rowSize * (desc.size.y - 1 - y);
			ptr = readScanline(ptr, desc.size.x, reinterpret_cast<vec4ub*>(rowPtr));
		}

		for (int32_t i = 0; i < desc.size.square(); ++i)
		{
			int8_t expo = rgbeData[i].w - 128;
			float scale = (expo > 0) ? static_cast<float>(1 << expo) : 1.0f / static_cast<float>(1 << -expo);
			float fx = scale * static_cast<float>(rgbeData[i].x);
			float fy = scale * static_cast<float>(rgbeData[i].y);
			float fz = scale * static_cast<float>(rgbeData[i].z);
			floatDataWrapper[i] = vec4(fx, fy, fz, 1.0f);
		}
	}
	else
	{
		desc.data.resize(desc.size.y * rowSize);
		for (int y = 0; y < desc.size.y; ++y)
		{
			auto rowPtr = desc.data.element_ptr((desc.size.y - 1 - y) * rowSize);
			ptr = readScanline(ptr, desc.size.x, reinterpret_cast<vec4ub*>(rowPtr));
		}
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
