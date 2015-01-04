/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/ddsloader.h>

using namespace et;

struct DDS_PIXELFORMAT
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

struct DDS_HEADER
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
};

enum DDSCAPS2
{
	DDSCAPS2_CUBEMAP = 0x200,
	DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
	DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
	DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
	DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
	DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
	DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
	DDSCAPS2_VOLUME = 0x200000
};

enum DDPF
{
	DDPF_ALPHAPIXELS = 0x1,
	DDPF_ALPHA = 0x2,
	DDPF_FOURCC = 0x4,
	DDPF_RGB = 0x40,
	DDPF_YUV = 0x200,
	DDPF_LUMINANCE = 0x20000
};

const uint32_t DDS_HEADER_ID = ET_COMPOSE_UINT32(' ', 'S', 'D', 'D');
const uint32_t FOURCC_DXT1 = ET_COMPOSE_UINT32('1', 'T', 'X', 'D');
const uint32_t FOURCC_DXT3 = ET_COMPOSE_UINT32('3', 'T', 'X', 'D');
const uint32_t FOURCC_DXT5 = ET_COMPOSE_UINT32('5', 'T', 'X', 'D');
const uint32_t FOURCC_ATI2 = ET_COMPOSE_UINT32('1', 'I', 'T', 'A');

void dds::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	uint32_t headerId = 0;
	source.read((char*)&headerId, sizeof(headerId));
	
	if (headerId != DDS_HEADER_ID)
	{
		log::error("Unable to load DDS. Invalid file signature.");
		return;
	}
	
	DDS_HEADER header = { };
	source.read(reinterpret_cast<char*>(&header), sizeof(header));
	
	desc.size = vec2i(static_cast<int>(header.dwWidth), static_cast<int>(header.dwHeight));
	desc.mipMapCount = (header.dwMipMapCount < 1) ? 1 : header.dwMipMapCount;
	desc.minimalSizeForCompressedFormat = vec2i(4);
	
	if (header.dwCaps2 & DDSCAPS2_CUBEMAP)
	{
		desc.target = TextureTarget::Texture_Cube;
		desc.layersCount = ((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) ? 1 : 0) +
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) ? 1 : 0) +
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) ? 1 : 0) +
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) ? 1 : 0) +
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) ? 1 : 0) +
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) ? 1 : 0);
	}
	else
	{
		desc.target = TextureTarget::Texture_2D;
		desc.layersCount = 1;
	}
		
	switch (header.ddspf.dwFourCC)
	{
		case 0:
		{
			bool isRGB = (desc.channels == 3);
			
			bool isBGR = (header.ddspf.dwBBitMask == 255);
			
			desc.channels = header.ddspf.dwRGBBitCount / 8;
			
			desc.bitsPerPixel = header.ddspf.dwRGBBitCount;
			
			desc.internalformat = isRGB ? TextureFormat::RGB : TextureFormat::RGBA;
			
			desc.format = isBGR ? (isRGB  ? TextureFormat::BGR : TextureFormat::BGRA) :
				(isRGB  ? TextureFormat::RGB : TextureFormat::RGBA);
			
			desc.type = DataType::UnsignedChar;
			break;
		}
			
		case 34:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RG16;
			desc.format = TextureFormat::RG;
			desc.type = DataType::UnsignedShort;
			break;
		}

		case 36:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RGBA16;
			desc.format = TextureFormat::RGBA;
			desc.type = DataType::UnsignedShort;
			break;
		}

		case 111:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::R16F;
			desc.format = TextureFormat::R;
			desc.type = DataType::Half;
			break;
		}

		case 112:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RG16F;
			desc.format = TextureFormat::RG;
			desc.type = DataType::Half;
			break;
		}

		case 114:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::R32F;
			desc.format = TextureFormat::R;
			desc.type = DataType::Float;
			break;
		}

		case 115:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::R32F;
			desc.format = TextureFormat::R;
			desc.type = DataType::Float;
			break;
		}

		case 113:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RGBA16F;
			desc.format = TextureFormat::RGBA;
			desc.type = DataType::Half;
			break;
		}

		case 116:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::RGBA32F;
			desc.format = TextureFormat::RGBA;
			desc.type = DataType::Float;
			break;
		}
			
		case FOURCC_DXT1:
		{
			bool hasAlpha = header.ddspf.dwFlags & DDPF_ALPHAPIXELS;
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 1 * desc.channels;
			desc.internalformat = hasAlpha ? TextureFormat::DXT1_RGB : TextureFormat::DXT1_RGBA;
			desc.format = hasAlpha ? TextureFormat::RGB : TextureFormat::RGBA;
			desc.type = DataType::UnsignedChar;
			break;
		}

		case FOURCC_DXT3:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = TextureFormat::DXT3;
			desc.format = TextureFormat::RGBA;
			desc.type = DataType::UnsignedChar;
			break;
		}

		case FOURCC_DXT5:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = TextureFormat::DXT5;
			desc.format = TextureFormat::RGBA;
			desc.type = DataType::UnsignedChar;
			break;
		}
			
		case FOURCC_ATI2:
		{
			desc.compressed = true;
			desc.channels = 2;
			desc.bitsPerPixel = 4 * desc.channels;
			desc.internalformat = TextureFormat::RGTC2;
			desc.format = TextureFormat::RG;
			desc.type = DataType::UnsignedChar;
			break;
		}

		default:
		{
			char fourcc_str[5] = { };
			etCopyMemory(fourcc_str, &header.ddspf.dwFourCC, 4);
			log::error("Unsupported FOURCC: %u, text: %s", header.ddspf.dwFourCC, fourcc_str);
			return;
		}
	};
}

void dds::loadFromStream(std::istream& source, TextureDescription& desc)
{
	if (source.fail())
	{
		log::error("Unable to load DDS image from stream: %s", desc.origin().c_str());
		return;
	}
	
	loadInfoFromStream(source, desc);
	
	size_t dataSize = desc.layersCount * desc.dataSizeForAllMipLevels();
	if (dataSize)
	{
		desc.data = BinaryDataStorage(dataSize);
		source.read(desc.data.binary(), static_cast<std::streamsize>(dataSize));
	}
	
	while ((desc.mipMapCount > 1) && ((desc.sizeForMipLevel(desc.mipMapCount - 1).x <= 4) ||
		(desc.sizeForMipLevel(desc.mipMapCount - 1).x <= 4)))
	{
		--desc.mipMapCount;
	}
}

void dds::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void dds::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}
