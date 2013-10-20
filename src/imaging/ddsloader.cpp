/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/stream.h>
#include <et/imaging/ddsloader.h>
#include <et/opengl/opengl.h>

using namespace et;

enum D3D10_RESOURCE_DIMENSION 
{
	D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
	D3D10_RESOURCE_DIMENSION_BUFFER = 1,
	D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
	D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
	D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
};

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
const uint32_t FOURCC_ATI1 = ET_COMPOSE_UINT32('1', 'I', 'T', 'A');
const uint32_t FOURCC_ATI2 = ET_COMPOSE_UINT32('1', 'I', 'T', 'A');

void DDSLoader::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	uint32_t headerId = 0;
	source.read((char*)&headerId, sizeof(headerId));

	if (headerId != DDS_HEADER_ID) return;

	DDS_HEADER header = { };
	source.read((char*)&header, sizeof(header));

	desc.size = vec2i(static_cast<int>(header.dwWidth), static_cast<int>(header.dwHeight));
	desc.mipMapCount = (header.dwMipMapCount < 1) ? 1 : header.dwMipMapCount;
	desc.minimalSizeForCompressedFormat = vec2i(4);

	if (header.dwCaps2 & DDSCAPS2_CUBEMAP)
	{
		desc.target = GL_TEXTURE_CUBE_MAP;
		desc.layersCount = 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) ? 1 : 0) + 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) ? 1 : 0) + 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) ? 1 : 0) + 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) ? 1 : 0) + 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) ? 1 : 0) + 
			((header.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) ? 1 : 0);
	}
	else
	{
		desc.target = GL_TEXTURE_2D;
		desc.layersCount = 1;
	}

	switch (header.ddspf.dwFourCC)
	{
        case 0:
        {
            desc.channels = header.ddspf.dwRGBBitCount / 8;
            desc.bitsPerPixel = header.ddspf.dwRGBBitCount;
            desc.internalformat = (desc.channels == 3) ? GL_RGB : GL_RGBA;
			
#if defined(GL_BGR)
			desc.format = (header.ddspf.dwBBitMask == 255) ?
				((desc.channels == 3) ? GL_BGR : GL_BGRA) : desc.internalformat;
#else
			desc.format = (header.ddspf.dwBBitMask == 255) ?
				((desc.channels == 3) ? GL_RGB : GL_BGRA) : desc.internalformat;
#endif
            desc.type = GL_UNSIGNED_BYTE;
            break;
        }
            
#if defined(GL_RG) && defined(GL_RG16)
	case 34:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = GL_RG16;
			desc.format = GL_RG;
			desc.type = GL_UNSIGNED_SHORT;
			break;   
		}
#endif
			
#if defined(GL_RGBA16)
	case 36:
		{
			desc.bitsPerPixel = 16 * desc.channels;
			desc.channels = 4;
			desc.internalformat = GL_RGBA16;
			desc.format = GL_RGBA;
			desc.type = GL_UNSIGNED_SHORT;
			break;   
		}
#endif
			
#if defined(GL_R16F)
	case 111: 
		{
			desc.channels = 1;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = GL_R16F;
			desc.format = GL_RED;
			desc.type = GL_HALF_FLOAT;
			break;   
		}
#endif
            
#if defined(GL_RG16F) 
	case 112:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = GL_RG16F;
			desc.format = GL_RG;
			desc.type = GL_HALF_FLOAT;
			break;   
		}
#endif
            
#if defined(GL_R32F)
	case 114:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = GL_R32F;
			desc.format = GL_RED;
			desc.type = GL_FLOAT;
			break;   
		}
#endif
            
#if defined(GL_RG32F)
	case 115:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = GL_RG32F;
			desc.format = GL_RG;
			desc.type = GL_FLOAT;
			break;   
		}		 
#endif
			
#if defined(GL_RGBA16F)
	case 113:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = GL_RGBA16F;
			desc.format = GL_RGBA;
			desc.type = GL_HALF_FLOAT;
			break;   
		}
#endif
			
#if defined(GL_RGBA32F)
	case 116:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = GL_RGBA32F;
			desc.format = GL_RGBA;
			desc.type = GL_FLOAT;
			break;   
		}
#endif
#if defined(GL_COMPRESSED_RGB_S3TC_DXT1_EXT) 
	case FOURCC_DXT1:
		{
			bool hasAlpha = header.ddspf.dwFlags & DDPF_ALPHAPIXELS;
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 1 * desc.channels;
			desc.internalformat = hasAlpha ? GL_COMPRESSED_RGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			desc.format = hasAlpha ? GL_RGB : GL_RGBA;
			desc.type = GL_UNSIGNED_BYTE;
			break;
		}
#endif
            
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
	case FOURCC_DXT3:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			desc.format = GL_RGBA;
			desc.type = GL_UNSIGNED_BYTE;
			break;
		}
#endif
            
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) 
	case FOURCC_DXT5:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			desc.format = GL_RGBA;
			desc.type = GL_UNSIGNED_BYTE;
			break;
		}
#endif
            
#if defined(GL_COMPRESSED_RG_RGTC2)
	case FOURCC_ATI2:
		{
			desc.compressed = true;
			desc.channels = 2;
			desc.bitsPerPixel = 4 * desc.channels;
			desc.internalformat = GL_COMPRESSED_RG_RGTC2;
			desc.format = GL_RGB;
			desc.type = GL_UNSIGNED_BYTE;
			break;
		}
#endif
            
	default: 
		{
			char fourcc_str[5] = { };
			etCopyMemory(fourcc_str, &header.ddspf.dwFourCC, 4);
			log::error("Unsupported FOURCC: %u, text: %s", header.ddspf.dwFourCC, fourcc_str);
			return;
		}
	};
}

void DDSLoader::loadFromStream(std::istream& source, TextureDescription& desc)
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
}

void DDSLoader::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void DDSLoader::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}
