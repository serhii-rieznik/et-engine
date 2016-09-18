/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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

enum DXGI_FORMAT : uint32_t
{
	DXGI_FORMAT_UNKNOWN                     = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
	DXGI_FORMAT_R32G32B32A32_UINT           = 3,
	DXGI_FORMAT_R32G32B32A32_SINT           = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
	DXGI_FORMAT_R32G32B32_FLOAT             = 6,
	DXGI_FORMAT_R32G32B32_UINT              = 7,
	DXGI_FORMAT_R32G32B32_SINT              = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
	DXGI_FORMAT_R16G16B16A16_UINT           = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
	DXGI_FORMAT_R16G16B16A16_SINT           = 14,
	DXGI_FORMAT_R32G32_TYPELESS             = 15,
	DXGI_FORMAT_R32G32_FLOAT                = 16,
	DXGI_FORMAT_R32G32_UINT                 = 17,
	DXGI_FORMAT_R32G32_SINT                 = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
	DXGI_FORMAT_R10G10B10A2_UINT            = 25,
	DXGI_FORMAT_R11G11B10_FLOAT             = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	DXGI_FORMAT_R8G8B8A8_UINT               = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
	DXGI_FORMAT_R8G8B8A8_SINT               = 32,
	DXGI_FORMAT_R16G16_TYPELESS             = 33,
	DXGI_FORMAT_R16G16_FLOAT                = 34,
	DXGI_FORMAT_R16G16_UNORM                = 35,
	DXGI_FORMAT_R16G16_UINT                 = 36,
	DXGI_FORMAT_R16G16_SNORM                = 37,
	DXGI_FORMAT_R16G16_SINT                 = 38,
	DXGI_FORMAT_R32_TYPELESS                = 39,
	DXGI_FORMAT_D32_FLOAT                   = 40,
	DXGI_FORMAT_R32_FLOAT                   = 41,
	DXGI_FORMAT_R32_UINT                    = 42,
	DXGI_FORMAT_R32_SINT                    = 43,
	DXGI_FORMAT_R24G8_TYPELESS              = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
	DXGI_FORMAT_R8G8_TYPELESS               = 48,
	DXGI_FORMAT_R8G8_UNORM                  = 49,
	DXGI_FORMAT_R8G8_UINT                   = 50,
	DXGI_FORMAT_R8G8_SNORM                  = 51,
	DXGI_FORMAT_R8G8_SINT                   = 52,
	DXGI_FORMAT_R16_TYPELESS                = 53,
	DXGI_FORMAT_R16_FLOAT                   = 54,
	DXGI_FORMAT_D16_UNORM                   = 55,
	DXGI_FORMAT_R16_UNORM                   = 56,
	DXGI_FORMAT_R16_UINT                    = 57,
	DXGI_FORMAT_R16_SNORM                   = 58,
	DXGI_FORMAT_R16_SINT                    = 59,
	DXGI_FORMAT_R8_TYPELESS                 = 60,
	DXGI_FORMAT_R8_UNORM                    = 61,
	DXGI_FORMAT_R8_UINT                     = 62,
	DXGI_FORMAT_R8_SNORM                    = 63,
	DXGI_FORMAT_R8_SINT                     = 64,
	DXGI_FORMAT_A8_UNORM                    = 65,
	DXGI_FORMAT_R1_UNORM                    = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
	DXGI_FORMAT_BC1_TYPELESS                = 70,
	DXGI_FORMAT_BC1_UNORM                   = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
	DXGI_FORMAT_BC2_TYPELESS                = 73,
	DXGI_FORMAT_BC2_UNORM                   = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
	DXGI_FORMAT_BC3_TYPELESS                = 76,
	DXGI_FORMAT_BC3_UNORM                   = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
	DXGI_FORMAT_BC4_TYPELESS                = 79,
	DXGI_FORMAT_BC4_UNORM                   = 80,
	DXGI_FORMAT_BC4_SNORM                   = 81,
	DXGI_FORMAT_BC5_TYPELESS                = 82,
	DXGI_FORMAT_BC5_UNORM                   = 83,
	DXGI_FORMAT_BC5_SNORM                   = 84,
	DXGI_FORMAT_B5G6R5_UNORM                = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	DXGI_FORMAT_BC6H_TYPELESS               = 94,
	DXGI_FORMAT_BC6H_UF16                   = 95,
	DXGI_FORMAT_BC6H_SF16                   = 96,
	DXGI_FORMAT_BC7_TYPELESS                = 97,
	DXGI_FORMAT_BC7_UNORM                   = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
	DXGI_FORMAT_AYUV                        = 100,
	DXGI_FORMAT_Y410                        = 101,
	DXGI_FORMAT_Y416                        = 102,
	DXGI_FORMAT_NV12                        = 103,
	DXGI_FORMAT_P010                        = 104,
	DXGI_FORMAT_P016                        = 105,
	DXGI_FORMAT_420_OPAQUE                  = 106,
	DXGI_FORMAT_YUY2                        = 107,
	DXGI_FORMAT_Y210                        = 108,
	DXGI_FORMAT_Y216                        = 109,
	DXGI_FORMAT_NV11                        = 110,
	DXGI_FORMAT_AI44                        = 111,
	DXGI_FORMAT_IA44                        = 112,
	DXGI_FORMAT_P8                          = 113,
	DXGI_FORMAT_A8P8                        = 114,
	DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
};

enum D3D10_RESOURCE_DIMENSION : uint32_t
{
	D3D10_RESOURCE_DIMENSION_UNKNOWN    = 0,
	D3D10_RESOURCE_DIMENSION_BUFFER     = 1,
	D3D10_RESOURCE_DIMENSION_TEXTURE1D  = 2,
	D3D10_RESOURCE_DIMENSION_TEXTURE2D  = 3,
	D3D10_RESOURCE_DIMENSION_TEXTURE3D  = 4
};

struct DDS_HEADER_DXT10
{
	DXGI_FORMAT dxgiFormat;
	D3D10_RESOURCE_DIMENSION resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
} ;


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
const uint32_t FOURCC_DX10 = ET_COMPOSE_UINT32('0', '1', 'X', 'D');

void fillDescriptionWithFormat(TextureDescription&, DXGI_FORMAT);

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
	
	desc.size = vec2i(static_cast<int32_t>(header.dwWidth), static_cast<int32_t>(header.dwHeight));
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
			desc.channels = header.ddspf.dwRGBBitCount / 8;
			desc.bitsPerPixel = header.ddspf.dwRGBBitCount;

			bool isRGB = (desc.channels == 3);
			bool isBGR = (header.ddspf.dwBBitMask == 255);
			desc.internalformat = isRGB ? TextureFormat::RGB : TextureFormat::RGBA;
			desc.format = isBGR ? (isRGB  ? TextureFormat::BGR : TextureFormat::BGRA) :
				(isRGB  ? TextureFormat::RGB : TextureFormat::RGBA);
			desc.type = DataFormat::UnsignedChar;
			break;
		}
			
		case 20:
		{
			desc.channels = header.ddspf.dwRGBBitCount / 8;
			desc.bitsPerPixel = header.ddspf.dwRGBBitCount;
			desc.internalformat = TextureFormat::RGB;
			desc.format = TextureFormat::BGR;
			desc.type = DataFormat::UnsignedChar;
			break;
		}
			
		case 34:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RG16;
			desc.format = TextureFormat::RG;
			desc.type = DataFormat::UnsignedShort;
			break;
		}

		case 36:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RGBA16;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::UnsignedShort;
			break;
		}

		case 111:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::R16F;
			desc.format = TextureFormat::R;
			desc.type = DataFormat::Half;
			break;
		}

		case 112:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RG16F;
			desc.format = TextureFormat::RG;
			desc.type = DataFormat::Half;
			break;
		}

		case 114:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::R32F;
			desc.format = TextureFormat::R;
			desc.type = DataFormat::Float;
			break;
		}

		case 115:
		{
			desc.channels = 2;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::R32F;
			desc.format = TextureFormat::R;
			desc.type = DataFormat::Float;
			break;
		}

		case 113:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 16 * desc.channels;
			desc.internalformat = TextureFormat::RGBA16F;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::Half;
			break;
		}

		case 116:
		{
			fillDescriptionWithFormat(desc, DXGI_FORMAT_R32G32B32A32_FLOAT);
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
			desc.type = DataFormat::UnsignedChar;
			break;
		}

		case FOURCC_DXT3:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = TextureFormat::DXT3;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::UnsignedChar;
			break;
		}

		case FOURCC_DXT5:
		{
			desc.compressed = true;
			desc.channels = 4;
			desc.bitsPerPixel = 2 * desc.channels;
			desc.internalformat = TextureFormat::DXT5;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::UnsignedChar;
			break;
		}
			
		case FOURCC_ATI2:
		{
			desc.compressed = true;
			desc.channels = 2;
			desc.bitsPerPixel = 4 * desc.channels;
			desc.internalformat = TextureFormat::RGTC2;
			desc.format = TextureFormat::RG;
			desc.type = DataFormat::UnsignedChar;
			break;
		}
			
		case FOURCC_DX10:
		{
			DDS_HEADER_DXT10 dx10Header = { };
			source.read(reinterpret_cast<char*>(&dx10Header), sizeof(dx10Header));
			fillDescriptionWithFormat(desc, dx10Header.dxgiFormat);
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
	
	uint32_t dataSize = desc.layersCount * desc.dataSizeForAllMipLevels();
	if (dataSize)
	{
		desc.data = BinaryDataStorage(dataSize);
		source.read(desc.data.binary(), dataSize);
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

/*
 * Service
 */
void fillDescriptionWithFormat(TextureDescription& desc, DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		{
			desc.channels = 1;
			desc.bitsPerPixel = 8 * desc.channels;
			desc.internalformat = TextureFormat::R8;
			desc.format = TextureFormat::R;
			desc.type = DataFormat::UnsignedChar;
			break;
		}
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 8 * desc.channels;
			desc.internalformat = TextureFormat::RGBA8;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::UnsignedChar;
			break;
		}
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		{
			desc.channels = 4;
			desc.bitsPerPixel = 32 * desc.channels;
			desc.internalformat = TextureFormat::RGBA32F;
			desc.format = TextureFormat::RGBA;
			desc.type = DataFormat::Float;
			break;
		}
			
		default:
		{
			ET_FAIL("Unhandled format");
			break;
		}
	}
}
