/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/bmploader.h>

using namespace et;

#pragma pack(push, 1)
struct FileHeader
{
	int8_t bfType[2];
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};

struct InfoHeader
{
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};
#pragma pack(pop)

void et::bmp::loadInfoFromStream(std::istream&, TextureDescription&)
{
	ET_FAIL("Not implemented");
}

void et::bmp::loadFromStream(std::istream& source, TextureDescription& desc)
{
	FileHeader fh = { };
	InfoHeader ih = { };
	source.read(reinterpret_cast<char*>(&fh), sizeof(FileHeader));
	source.read(reinterpret_cast<char*>(&ih), sizeof(InfoHeader));

	desc.target = TextureTarget::Texture_2D;
	desc.type = DataType::UnsignedChar;
	desc.size = vec2i(ih.biWidth, ih.biHeight);
	desc.bitsPerPixel = ih.biBitCount;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.channels = desc.bitsPerPixel/ 8;

	if (desc.channels == 3)
	{
		desc.internalformat = TextureFormat::RGB;
		desc.format = TextureFormat::RGB;
	}
	else if (desc.channels == 4)
	{
		desc.internalformat = TextureFormat::RGBA;
		desc.format = TextureFormat::RGBA;
	}
	else 
	{
		ET_FAIL_FMT("Unsupported bitmap format: %u", desc.channels);
	}

	desc.data.resize(desc.size.square() * desc.channels);

	size_t extraOffset = fh.bfOffBits - (sizeof(FileHeader) + sizeof(InfoHeader));
	if (extraOffset > 0)
		source.seekg(extraOffset, std::ios::cur);

	source.read(desc.data.binary(), desc.data.size());

	unsigned char* pixels = desc.data.data();
	for (size_t i = 0; i < desc.data.size(); i += desc.channels)
	{
		pixels[i] ^= pixels[i+2];
		pixels[i+2] ^= pixels[i];
		pixels[i] ^= pixels[i+2];
	}
}

void et::bmp::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void et::bmp::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}
