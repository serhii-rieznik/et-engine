/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/geometry/geometry.h>
#include <et/imaging/pvrloader.h>

#include <external/pvr/PVRTTexture.h>
#include <external/pvr/PVRTDecompress.h>

using namespace et;

uint32_t getBitsPerPixel(PVRTuint64 u64PixelFormat);

void parseTextureFormat(const PVRTextureHeaderV3&, TextureDescription&);
void loadInfoFromV3Header(const PVRTextureHeaderV3&, const BinaryDataStorage&, TextureDescription&);

void pvr::loadInfoFromStream(std::istream& stream, TextureDescription& desc)
{
	std::istream::off_type offset = stream.tellg();

	desc.dataLayout = TextureDataLayout::MipsFirst;

	PVR_Texture_Header header2 = {};
	stream.read(reinterpret_cast<char*>(&header2), sizeof(header2));
	if (header2.dwPVR == ET_COMPOSE_UINT32_INVERTED('P', 'V', 'R', '!'))
	{
		ET_FAIL("Legacy PVR files are not supported anymore.")
	}
	else
	{
		stream.seekg(offset, std::ios_base::beg);
		PVRTextureHeaderV3 header3 = {};
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
	stream.read(desc.data.binary(), desc.data.dataSize());

	bool decompress = (desc.format >= TextureFormat::PVR_2bpp_RGB) && (desc.format <= TextureFormat::PVR_4bpp_sRGBA);
	if (decompress)
	{
		BinaryDataStorage rgbaData(desc.size.square() * 4, 0);
		PVRTDecompressPVRTC(desc.data.data(), (bitsPerPixelForTextureFormat(desc.format) == 2), desc.size.x, desc.size.y, rgbaData.data());
		desc.levelCount = 1;
		desc.format = TextureFormat::RGBA8;
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
		switch (PixelFormat)
		{
		case ePVRTPF_PVRTCI_2bpp_RGB:
		{
			desc.format = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ? TextureFormat::PVR_2bpp_sRGB : TextureFormat::PVR_2bpp_RGB;
			return;
		}

		case ePVRTPF_PVRTCI_2bpp_RGBA:
		{
			desc.format = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ? TextureFormat::PVR_2bpp_sRGBA : TextureFormat::PVR_2bpp_RGBA;
			return;
		}

		case ePVRTPF_PVRTCI_4bpp_RGB:
		{
			desc.format = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ? TextureFormat::PVR_4bpp_sRGB : TextureFormat::PVR_4bpp_RGB;
			return;
		}

		case ePVRTPF_PVRTCI_4bpp_RGBA:
		{
			desc.format = (sTextureHeader.u32ColourSpace == ePVRTCSpacesRGB) ? TextureFormat::PVR_4bpp_sRGBA : TextureFormat::PVR_4bpp_RGBA;
			return;
		}

		default:
			ET_FAIL_FMT("Invalid PVR compressed format: %llu", PixelFormat)
		}
	}
	else
	{
		switch (ChannelType)
		{
		case ePVRTVarTypeFloat:
		{
			switch (PixelFormat)
			{
			case PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16):
			{
				desc.format = TextureFormat::RGBA16F;
				return;
			}

			case PVRTGENPIXELID3('r', 'g', 'b', 16, 16, 16):
			{
				ET_FAIL("Not implemented");
				/*
				desc.type = DataFormat::Half;
				desc.format = TextureFormat::RGB16F;
				desc.channels = 3;
				*/
				return;
			}

			case PVRTGENPIXELID2('l', 'a', 16, 16):
			{
				desc.format = TextureFormat::RG16F;
				return;
			}

			case PVRTGENPIXELID1('l', 16):
			case PVRTGENPIXELID1('a', 16):
			{
				desc.format = TextureFormat::R16F;
				return;
			}

			case PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32):
			{
				desc.format = TextureFormat::RGBA32F;
				return;
			}

			case PVRTGENPIXELID3('r', 'g', 'b', 32, 32, 32):
			{
				ET_FAIL("Not implemented");
				/*
				desc.type = DataFormat::Float;
				desc.internalformat = TextureFormat::RGB32F;
				desc.format = TextureFormat::RGB;
				desc.channels = 3;
				*/
				return;
			}

			case PVRTGENPIXELID2('l', 'a', 32, 32):
			{
				desc.format = TextureFormat::RG32F;
				return;
			}

			case PVRTGENPIXELID1('l', 32):
			case PVRTGENPIXELID1('a', 32):
			{
				desc.format = TextureFormat::R32F;
				return;
			}
			}
			break;
		}

		case ePVRTVarTypeUnsignedByteNorm:
		{
			switch (PixelFormat)
			{
			case PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8):
			{
				desc.format = TextureFormat::RGBA8;
				return;
			}
			case PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8):
			{
				ET_FAIL("Not implemented");
				/*
				desc.internalformat = TextureFormat::RGB;
				desc.format = TextureFormat::RGB;
				desc.channels = 3;
				*/
				return;
			}

			case PVRTGENPIXELID2('l', 'a', 8, 8):
			{
				desc.format = TextureFormat::RG8;
				return;
			}

			case PVRTGENPIXELID1('a', 8):
			case PVRTGENPIXELID1('l', 8):
			{
				desc.format = TextureFormat::R8;
				return;
			}

			case PVRTGENPIXELID4('b', 'g', 'r', 'a', 8, 8, 8, 8):
			{
				desc.format = TextureFormat::BGRA8;
				return;
			}
			}
			break;
		}

		case ePVRTVarTypeUnsignedShortNorm:
		{
			switch (PixelFormat)
			{
			case PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5):
			{
				desc.format = TextureFormat::RGB565;
				return;
			}
			}
			break;
		}

		default:
			ET_FAIL("Invalid PVR texture format.")
		}
	}
}

void loadInfoFromV3Header(const PVRTextureHeaderV3& header, const BinaryDataStorage&, TextureDescription& desc)
{
	desc.layersCount = header.u32NumFaces;
	ET_ASSERT((desc.layersCount == 1) || (desc.layersCount == 6));

	desc.size = vec2i(header.u32Width, header.u32Height);
	desc.levelCount = (header.u32MIPMapCount > 0) ? header.u32MIPMapCount : 1;
	desc.target = (desc.layersCount == 6) ? TextureTarget::Texture_Cube : TextureTarget::Texture_2D;

	parseTextureFormat(header, desc);
}
