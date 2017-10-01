/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <external/libpng/png.h>
#include <et/imaging/pngloader.h>

using namespace et;

void parseFormat(TextureDescription& desc, png_structp pngPtr, png_infop infoPtr, png_size_t* rowBytes, uint32_t& channels);

void streamReadData(png_structp pngPtr, png_bytep data, png_size_t length);

void handlePngError(png_structp, png_const_charp);
void handlePngWarning(png_structp, png_const_charp);

void et::png::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	static const int PNGSIGSIZE = 8;

	png_byte pngsig[PNGSIGSIZE] = { };
	source.read((char*)pngsig, PNGSIGSIZE);

	if (png_sig_cmp(pngsig, 0, PNGSIGSIZE))
	{
		log::error("Couldn't recognize PNG");
		return;
	}

	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
		handlePngError, handlePngWarning);
	
	if (pngPtr == nullptr)
	{
		log::error("Couldn't initialize png read struct");
		return;
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) 
	{
		log::error("Couldn't initialize png info struct");
		png_destroy_read_struct(&pngPtr, (png_infopp)0, (png_infopp)0);    
		return;
	}

	uint32_t channels = 0;
	png_set_read_fn(pngPtr, (png_voidp)&source, streamReadData);
	png_set_sig_bytes(pngPtr, PNGSIGSIZE);
	png_read_info(pngPtr, infoPtr); 
	parseFormat(desc, pngPtr, infoPtr, nullptr, channels);
	png_destroy_info_struct(pngPtr, &infoPtr);
	png_destroy_read_struct(&pngPtr, 0, 0);
}

void et::png::loadFromStream(std::istream& source, TextureDescription& desc, bool flip)
{
	static const int PNGSIGSIZE = 8;

	png_byte pngsig[PNGSIGSIZE] = { };
	source.read((char*)pngsig, PNGSIGSIZE);

	if (png_sig_cmp(pngsig, 0, PNGSIGSIZE))
	{
		log::error("Couldn't recognize PNG");
		return;
	}

	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
		handlePngError, handlePngWarning);
	png_infop infoPtr = nullptr;
	
	if (pngPtr == nullptr)
	{
		log::error("Couldn't initialize png read struct");
		return;
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
		return;
	}
	
	infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == nullptr)
	{
		log::error("Couldn't initialize png info struct");
		png_destroy_read_struct(&pngPtr, nullptr, nullptr);
		return;
	}

	uint32_t channels = 0;
	png_size_t rowBytes = 0;

	png_set_read_fn(pngPtr, (png_voidp)&source, streamReadData);
	png_set_sig_bytes(pngPtr, PNGSIGSIZE);
	png_read_info(pngPtr, infoPtr); 
	parseFormat(desc, pngPtr, infoPtr, &rowBytes, channels);

	uint32_t bpp = bitsPerPixelForTextureFormat(desc.format) / 8;
	desc.data.resize(static_cast<uint32_t>(desc.size.square()) * bpp);
	png_bytepp row_pointers = reinterpret_cast<png_bytepp>(sharedBlockAllocator().allocate(sizeof(png_bytep) * desc.size.y));
	png_bytep ptr0 = desc.data.data();
	
	if (flip)
	{
		for (png_size_t i = 0, e = static_cast<png_size_t>(desc.size.y); i < e; ++i)
			row_pointers[i] = ptr0 + (e - 1 - i) * rowBytes;
	}
	else
	{
		for (png_size_t i = 0, e = static_cast<png_size_t>(desc.size.y); i < e; ++i)
			row_pointers[i] = ptr0 + i * rowBytes;
	}

	png_read_image(pngPtr, row_pointers);
	png_destroy_info_struct(pngPtr, &infoPtr);
	png_destroy_read_struct(&pngPtr, 0, 0);

	if (bpp == 16 * channels)
	{
		uint16_t* data_ptr = reinterpret_cast<uint16_t*>(desc.data.binary());
		for (size_t i = 0; i < desc.data.dataSize() / 2; ++i)
		{
			uint16_t value = data_ptr[i];
			data_ptr[i] = static_cast<uint16_t>(((value >> 8) & 0xff) + ((value << 8) & 0xff));
		}
	}

	if (channels == 3)
	{
		// convert RGB to RGBA
		BinaryDataStorage rgbData = desc.data;
		vec3ub* rgb = reinterpret_cast<vec3ub*>(rgbData.data());
		vec4ub* rgba = reinterpret_cast<vec4ub*>(desc.data.data());
		for (uint64_t i = 0, e = desc.data.size() / 4; i < e; ++i)
		{
			*rgba = vec4ub(*rgb, 255);
			++rgb;
			++rgba;
		}
	}

	sharedBlockAllocator().release(row_pointers);
}

void et::png::loadFromFile(const std::string& path, TextureDescription& desc, bool flip)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc, flip);
	}
}

void et::png::loadInfoFromFile(const std::string& path, TextureDescription& desc)
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
void parseFormat(TextureDescription& desc, png_structp pngPtr, png_infop infoPtr, png_size_t* rowBytes, uint32_t& channels)
{
	desc.levelCount = 1;
	desc.layerCount = 1;
	desc.size.x = static_cast<int32_t>(png_get_image_width(pngPtr, infoPtr));
	desc.size.y = static_cast<int32_t>(png_get_image_height(pngPtr, infoPtr));
	
	channels = png_get_channels(pngPtr, infoPtr);
	int color_type = png_get_color_type(pngPtr, infoPtr); 
	int interlace_method = png_get_interlace_type(pngPtr, infoPtr);
	int compression = png_get_compression_type(pngPtr, infoPtr);
	int filter = png_get_filter_type(pngPtr, infoPtr);
	
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(pngPtr);
		channels = 3;
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY)
	{
		channels = 1;
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		channels = 2;
	}
	
	if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) 
	{        
		png_set_tRNS_to_alpha(pngPtr);        
		channels += 1;
	}    
	
	int bpp = 0;
	png_read_update_info(pngPtr, infoPtr);
	png_get_IHDR(pngPtr, infoPtr, (png_uint_32p)&desc.size.x, (png_uint_32p)&desc.size.y, 
				 &bpp, &color_type, &interlace_method, &compression, &filter);
		
	if (rowBytes)
		*rowBytes = png_get_rowbytes(pngPtr, infoPtr);

	switch (channels)
	{
		case 1:
		{
			desc.format = (bpp == 16) ? TextureFormat::R16 : TextureFormat::R8;
			break;
		};

		case 2:
		{
			desc.format = (bpp == 16) ? TextureFormat::RG16 : TextureFormat::RG8;
			break;
		}

		case 3:
		{
			desc.format = (bpp == 16) ? TextureFormat::RGBA16 : TextureFormat::RGBA8;
			break;
		}

		case 4:
		{ 
			desc.format = (bpp == 16) ? TextureFormat::RGBA16 : TextureFormat::RGBA8;
			break;
		}

		default: 
			ET_FAIL_FMT("Unknown PNG texture format with %u channels", channels);
	}
}

void streamReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
	reinterpret_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data, length);
}

void handlePngError(png_structp, png_const_charp e)
{
	log::error("[PNGLoader] %s", e);
}

void handlePngWarning(png_structp, png_const_charp)
{
}
