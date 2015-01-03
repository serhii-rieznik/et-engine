/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et/geometry/geometry.h>
#include <et/imaging/pvrloader.h>
#include <pvr/PVRTTexture.h>
#include <pvr/PVRTDecompress.h>

using namespace et;

uint32_t getBitsPerPixel(PVRTuint64 u64PixelFormat);

void parseTextureFormat(const PVRTextureHeaderV3&, TextureDescription&);
void loadInfoFromV3Header(const PVRTextureHeaderV3&, const BinaryDataStorage&, TextureDescription&);

void pvr::loadInfoFromStream(std::istream& stream, TextureDescription& desc)
{
    std::istream::off_type offset = stream.tellg();
	
	desc.minimalDataSize = 32;
	desc.dataLayout = TextureDataLayout::MipsFirst;
    
	PVR_Texture_Header header2 = { };
	stream.read(reinterpret_cast<char*>(&header2), sizeof(header2));
	if (header2.dwPVR == ET_COMPOSE_UINT32_INVERTED('P', 'V', 'R', '!'))
    {
		ET_FAIL("Legacy PVR files are not supported anymore.")
    }
    else 
    {
        stream.seekg(offset, std::ios_base::beg);
        PVRTextureHeaderV3 header3 = { };
		BinaryDataStorage meta;
        stream.read(reinterpret_cast<char*>(&header3), sizeof(header3));
        
        if (header3.u32Version == PVRTEX3_IDENT)
        {
			if (header3.u32MetaDataSize > 0)
			{
				meta.resize(header3.u32MetaDataSize);
				stream.read(meta.binary(), header3.u32MetaDataSize);
			}
			
			loadInfoFromV3Header(header3, meta, desc);
        }
        else
		{
			log::error("Unrecognized PVR input stream");
		}
    }
}

void pvr::loadFromStream(std::istream& stream, TextureDescription& desc)
{
	loadInfoFromStream(stream, desc);
	desc.data.resize(desc.layersCount * desc.dataSizeForAllMipLevels());
	stream.read(desc.data.binary(), static_cast<std::streamsize>(desc.data.dataSize()));
	
#if (ET_PLATFORM_IOS)
	
	bool decompress = false;
	
#else 
	
	bool decompress = (desc.internalformat >= TextureFormat::PVR_2bpp_RGB) &&
		(desc.internalformat <= TextureFormat::PVR_4bpp_sRGBA);
	
#endif
	
	if (decompress)
	{
		BinaryDataStorage rgbaData(desc.size.square() * 4, 0);
		
		PVRTDecompressPVRTC(desc.data.data(), (desc.bitsPerPixel == 2), desc.size.x, desc.size.y, rgbaData.data());
		
		desc.mipMapCount = 1;
		desc.internalformat = TextureFormat::RGBA;
		desc.format = TextureFormat::RGBA;
		desc.type = DataType::UnsignedChar;
		desc.compressed = 0;
		desc.data = rgbaData;
	}
}

void pvr::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}

void pvr::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

/*
 * Service functions
 */

// Generate a 4 channel PixelID.
#define PVRTGENPIXELID4(C1Name, C2Name, C3Name, C4Name, C1Bits, C2Bits, C3Bits, C4Bits) \
	(((PVRTuint64)C1Name) + ((PVRTuint64)C2Name << 8) + ((PVRTuint64)C3Name << 16) + ((PVRTuint64)C4Name << 24) + \
	((PVRTuint64)C1Bits << 32) + ((PVRTuint64)C2Bits << 40) + ((PVRTuint64)C3Bits << 48) + ((PVRTuint64)C4Bits << 56))

// Generate a 1 channel PixelID.
#define PVRTGENPIXELID3(C1Name, C2Name, C3Name, C1Bits, C2Bits, C3Bits) \
	(PVRTGENPIXELID4(C1Name, C2Name, C3Name, 0, C1Bits, C2Bits, C3Bits, 0))

// Generate a 2 channel PixelID.
#define PVRTGENPIXELID2(C1Name, C2Name, C1Bits, C2Bits) \
	(PVRTGENPIXELID4(C1Name, C2Name, 0, 0, C1Bits, C2Bits, 0, 0))

// Generate a 3 channel PixelID.
#define PVRTGENPIXELID1(C1Name, C1Bits) \
	(PVRTGENPIXELID4(C1Name, 0, 0, 0, C1Bits, 0, 0, 0))

void parseTextureFormat(const PVRTextureHeaderV3& sTextureHeader, TextureDescription& desc)
{
	PVRTuint64 PixelFormat = sTextureHeader.u64PixelFormat;
	EPVRTVariableType ChannelType = static_cast<EPVRTVariableType>(sTextureHeader.u32ChannelType);
		
	if ((PixelFormat & PVRTEX_PFHIGHMASK) == 0)
	{
		desc.type = DataType::UnsignedChar;
		
		switch (PixelFormat)
		{
			case ePVRTPF_PVRTCI_2bpp_RGB:
			{
				desc.channels = 3;
				desc.format = TextureFormat::RGB;
				desc.internalformat = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ?
					TextureFormat::PVR_2bpp_sRGB : TextureFormat::PVR_2bpp_RGB;
				desc.compressed = true;
				return;
			}
				
			case ePVRTPF_PVRTCI_2bpp_RGBA:
			{
				desc.channels = 4;
				desc.format = TextureFormat::RGBA;
				desc.internalformat = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ?
					TextureFormat::PVR_2bpp_sRGBA : TextureFormat::PVR_2bpp_RGBA;
				desc.compressed = true;
				return;
			}
				
			case ePVRTPF_PVRTCI_4bpp_RGB:
			{
				desc.channels = 3;
				desc.format = TextureFormat::RGB;
				desc.internalformat = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ?
					TextureFormat::PVR_4bpp_sRGB : TextureFormat::PVR_4bpp_RGB;
				desc.compressed = true;
				return;
			}

			case ePVRTPF_PVRTCI_4bpp_RGBA:
			{
				desc.channels = 4;
				desc.format = TextureFormat::RGBA;
				desc.internalformat = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ?
					TextureFormat::PVR_4bpp_sRGBA : TextureFormat::PVR_4bpp_RGBA;
				desc.compressed = true;
				return;
			}
				
			default:
				ET_FAIL_FMT("Invalid PVR compressed format: %llu", PixelFormat)
		}
	}
	else
	{
		desc.compressed = 0;
		
		switch (ChannelType)
		{
			case ePVRTVarTypeFloat:
			{
				switch (PixelFormat)
				{
					case PVRTGENPIXELID4('r','g','b','a',16,16,16,16):
					{
						desc.type = DataType::Half;
						desc.internalformat = TextureFormat::RGBA16F;
						desc.format = TextureFormat::RGBA;
						desc.channels = 4;
						return;
					}
						
					case PVRTGENPIXELID3('r','g','b',16,16,16):
					{
						desc.type = DataType::Half;
						desc.internalformat = TextureFormat::RGB16F;
						desc.format = TextureFormat::RGB;
						desc.channels = 3;
						return;
					}
						
					case PVRTGENPIXELID2('l','a',16,16):
					{
						desc.type = DataType::Half;
						desc.internalformat = TextureFormat::RG16F;
						desc.format = TextureFormat::RG;
						desc.channels = 2;
						return;
					}
						
					case PVRTGENPIXELID1('l',16):
					{
						desc.type = DataType::Half;
						desc.internalformat = TextureFormat::R16F;
						desc.format = TextureFormat::R;
						desc.channels = 1;
						return;
					}
						
					case PVRTGENPIXELID1('a',16):
					{
						desc.type = DataType::Half;
						desc.format = TextureFormat::R16F;
						desc.internalformat = TextureFormat::R;
						desc.channels = 1;
						return;
					}
						
					case PVRTGENPIXELID4('r','g','b','a',32,32,32,32):
					{
						desc.type = DataType::Float;
						desc.internalformat = TextureFormat::RGBA32F;
						desc.format = TextureFormat::RGBA;
						desc.channels = 4;
						return;
					}
						
					case PVRTGENPIXELID3('r','g','b',32,32,32):
					{
						desc.type = DataType::Float;
						desc.internalformat = TextureFormat::RGB32F;
						desc.format = TextureFormat::RGB;
						desc.channels = 3;
						return;
					}
						
					case PVRTGENPIXELID2('l','a',32,32):
					{
						desc.type = DataType::Float;
						desc.internalformat = TextureFormat::RG32F;
						desc.format = TextureFormat::RG;
						desc.channels = 2;
						return;
					}
						
					case PVRTGENPIXELID1('l',32):
					{
						desc.type = DataType::Float;
						desc.internalformat = TextureFormat::R32F;
						desc.format = TextureFormat::R;
						desc.channels = 1;
						return;
					}
						
					case PVRTGENPIXELID1('a',32):
					{
						desc.type = DataType::Float;
						desc.internalformat = TextureFormat::R32F;
						desc.format = TextureFormat::R;
						desc.channels = 1;
						return;
					}
				}
				break;
			}
				
			case ePVRTVarTypeUnsignedByteNorm:
			{
				desc.type = DataType::UnsignedChar;
				switch (PixelFormat)
				{
					case PVRTGENPIXELID4('r','g','b','a',8,8,8,8):
					{
						desc.internalformat = TextureFormat::RGBA;
						desc.format = TextureFormat::RGBA;
						desc.channels = 4;
						return;
					}
					case PVRTGENPIXELID3('r','g','b',8,8,8):
					{
						desc.internalformat = TextureFormat::RGB;
						desc.format = TextureFormat::RGB;
						desc.channels = 3;
						return;
					}
						
					case PVRTGENPIXELID2('l','a',8,8):
					{
						desc.internalformat = TextureFormat::RG;
						desc.format = TextureFormat::RG8;
						desc.channels = 2;
						return;
					}

					case PVRTGENPIXELID1('l',8):
					{
						desc.format = TextureFormat::R8;
						desc.internalformat = TextureFormat::R;
						desc.channels = 1;
						return;
					}
						
					case PVRTGENPIXELID1('a',8):
					{
						desc.internalformat = TextureFormat::R;
						desc.format = TextureFormat::R8;
						desc.channels = 1;
						return;
					}
						
					case PVRTGENPIXELID4('b','g','r','a',8,8,8,8):
					{
						desc.internalformat = TextureFormat::BGRA;
						desc.format = TextureFormat::BGRA;
						desc.channels = 4;
						return;
					}
				}
				break;
			}

			case ePVRTVarTypeUnsignedShortNorm:
			{
				switch (PixelFormat)
				{
					case PVRTGENPIXELID4('r','g','b','a',4,4,4,4):
					{
						desc.type = DataType::UnsignedShort_4444;
						desc.internalformat = TextureFormat::RGBA;
						desc.format = TextureFormat::RGBA;
						desc.channels = 4;
						return;
					}
						
					case PVRTGENPIXELID4('r','g','b','a',5,5,5,1):
					{
						desc.type = DataType::UnsignedShort_5551;
						desc.internalformat = TextureFormat::RGBA;
						desc.format = TextureFormat::RGBA;
						desc.channels = 4;
						return;
					}
						
					case PVRTGENPIXELID3('r','g','b',5,6,5):
					{
						desc.type = DataType::UnsignedShort_565;
						desc.internalformat = TextureFormat::RGB;
						desc.format = TextureFormat::RGB;
						desc.channels = 3;
						return;
					}
				}
				break;
			}
				
			default:
				ET_FAIL("Invalid PVR texture format.")
				return;
		}
	}
}

uint32_t getBitsPerPixel(PVRTuint64 u64PixelFormat)
{
	if (u64PixelFormat & PVRTEX_PFHIGHMASK)
	{
		PVRTuint8* PixelFormatChar = reinterpret_cast<PVRTuint8*>(&u64PixelFormat);
		return PixelFormatChar[4] + PixelFormatChar[5] + PixelFormatChar[6] + PixelFormatChar[7];
	}
	
	switch (u64PixelFormat)
	{
		case ePVRTPF_BW1bpp:
			return 1;
			
		case ePVRTPF_PVRTCI_2bpp_RGB:
		case ePVRTPF_PVRTCI_2bpp_RGBA:
		case ePVRTPF_PVRTCII_2bpp:
			return 2;
			
		case ePVRTPF_PVRTCI_4bpp_RGB:
		case ePVRTPF_PVRTCI_4bpp_RGBA:
		case ePVRTPF_PVRTCII_4bpp:
		case ePVRTPF_ETC1:
		case ePVRTPF_EAC_R11:
		case ePVRTPF_ETC2_RGB:
		case ePVRTPF_ETC2_RGB_A1:
		case ePVRTPF_DXT1:
		case ePVRTPF_BC4:
			return 4;
			
		case ePVRTPF_DXT2:
		case ePVRTPF_DXT3:
		case ePVRTPF_DXT4:
		case ePVRTPF_DXT5:
		case ePVRTPF_BC5:
		case ePVRTPF_EAC_RG11:
		case ePVRTPF_ETC2_RGBA:
			return 8;
			
		case ePVRTPF_YUY2:
		case ePVRTPF_UYVY:
		case ePVRTPF_RGBG8888:
		case ePVRTPF_GRGB8888:
			return 16;
			
		case ePVRTPF_SharedExponentR9G9B9E5:
			return 32;
			
		default:
			ET_FAIL_FMT("Invalid PVR pixel format: %llu", u64PixelFormat);
	}
	
	return 0;
}

void loadInfoFromV3Header(const PVRTextureHeaderV3& header, const BinaryDataStorage&, TextureDescription& desc)
{
	desc.layersCount = header.u32NumFaces;
	ET_ASSERT((desc.layersCount == 1) || (desc.layersCount == 6));
	
	desc.size = vec2i(header.u32Width, header.u32Height);
	desc.mipMapCount = (header.u32MIPMapCount > 0) ? header.u32MIPMapCount : 1;
	desc.target = (desc.layersCount == 6) ? TextureTarget::Texture_Cube : TextureTarget::Texture_2D;
	desc.bitsPerPixel = getBitsPerPixel(header.u64PixelFormat);
	
	parseTextureFormat(header, desc);
}
