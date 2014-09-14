/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <libpng/png.h>
#include <et/opengl/opengl.h>
#include <et/imaging/pngloader.h>

using namespace et;

void parseFormat(TextureDescription& desc, png_structp pngPtr, png_infop infoPtr, png_size_t* rowBytes);

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

	png_set_read_fn(pngPtr, (png_voidp)&source, streamReadData);
	png_set_sig_bytes(pngPtr, PNGSIGSIZE);
	png_read_info(pngPtr, infoPtr); 
	parseFormat(desc, pngPtr, infoPtr, 0);
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

	png_size_t rowBytes = 0;

	png_set_read_fn(pngPtr, (png_voidp)&source, streamReadData);
	png_set_sig_bytes(pngPtr, PNGSIGSIZE);
	png_read_info(pngPtr, infoPtr); 
	parseFormat(desc, pngPtr, infoPtr, &rowBytes);

	desc.data = BinaryDataStorage(static_cast<size_t>(desc.size.square()) * desc.bitsPerPixel / 8);
	png_bytepp rowPtrs = new png_bytep[desc.size.y]; 
	png_bytep ptr0 = desc.data.data();
	
	if (flip)
	{
		for (png_size_t i = 0, e = static_cast<png_size_t>(desc.size.y); i < e; ++i)
			rowPtrs[i] = ptr0 + (e - 1 - i) * rowBytes;
	}
	else
	{
		for (png_size_t i = 0, e = static_cast<png_size_t>(desc.size.y); i < e; ++i)
			rowPtrs[i] = ptr0 + i * rowBytes;
	}

	png_read_image(pngPtr, rowPtrs); 

	png_destroy_info_struct(pngPtr, &infoPtr);
	png_destroy_read_struct(&pngPtr, 0, 0);

	if (desc.bitsPerPixel / desc.channels == 16)
	{
		unsigned short* data_ptr = reinterpret_cast<unsigned short*>(desc.data.binary());
		for (size_t i = 0; i < desc.data.dataSize() / 2; ++i)
		{
			unsigned short value = data_ptr[i];
			data_ptr[i] = static_cast<unsigned short>(((value >> 8) & 0xff) + ((value << 8) & 0xff));
		}
	}

	delete [] rowPtrs;
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

void parseFormat(TextureDescription& desc, png_structp pngPtr, png_infop infoPtr, png_size_t* rowBytes)
{
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.size.x = static_cast<int>(png_get_image_width(pngPtr, infoPtr));
	desc.size.y = static_cast<int>(png_get_image_height(pngPtr, infoPtr));
	desc.channels = png_get_channels(pngPtr, infoPtr);
	
	int color_type = png_get_color_type(pngPtr, infoPtr); 
	int interlace_method = png_get_interlace_type(pngPtr, infoPtr);
	int compression = png_get_compression_type(pngPtr, infoPtr);
	int filter = png_get_filter_type(pngPtr, infoPtr);
	
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(pngPtr);
		desc.channels = 3;
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY)
	{
		desc.channels = 1;
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		desc.channels = 2;
	}
	
	if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) 
	{        
		png_set_tRNS_to_alpha(pngPtr);        
		desc.channels += 1;
	}    
	
	int bpp = 0;
	png_read_update_info(pngPtr, infoPtr);
	png_get_IHDR(pngPtr, infoPtr, (png_uint_32p)&desc.size.x, (png_uint_32p)&desc.size.y, 
				 &bpp, &color_type, &interlace_method, &compression, &filter);
	
	desc.bitsPerPixel = desc.channels * static_cast<size_t>(bpp);
	desc.type = (bpp == 16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
	
	if (rowBytes)
		*rowBytes = png_get_rowbytes(pngPtr, infoPtr);
	
	switch (desc.channels)
	{
		case 1:
		{
#if defined(GL_LUMINANCE)
			if (bpp == 8)
			{
				desc.internalformat = GL_LUMINANCE;
				desc.format = GL_LUMINANCE;
			}
			else
			{
				ET_FAIL("Unsupported PNG format");
			}
#elif defined(GL_R8) && defined(GL_R16)
			if (bpp == 8)
			{
				desc.internalformat = GL_R8;
			}
			else if (bpp == 16)
			{
				desc.internalformat = GL_R16;
			}
			desc.format = GL_RED;
#else
#			error Unable to resolve OpenGL format for 1-channel texture
#endif
			break;
		};

		case 2:
		{
#if defined(GL_RG8) && defined(GL_RG16)

			desc.internalformat = (bpp == 16) ? GL_RG16 : GL_RG8;
			desc.format = GL_RG;

#elif defined(GL_LUMINANCE_ALPHA)

			desc.internalformat = GL_LUMINANCE_ALPHA;
			desc.format = GL_LUMINANCE_ALPHA;

#else
#			error Unable to resolve OpenGL format for 1-channel texture
#endif
			break;
		}

		case 3:
		{
#if defined(GL_RGB16)
			desc.internalformat = (bpp == 16) ? GL_RGB16 : GL_RGB;
#else
			desc.internalformat = GL_RGB;
#endif
			desc.format = GL_RGB;
			break;
		}

		case 4:
		{ 
#if defined(GL_RGBA16)
			desc.internalformat = (bpp == 16) ? GL_RGBA16 : GL_RGBA;
#else
			desc.internalformat = GL_RGBA;
#endif
			desc.format = GL_RGBA;
			break;
		}

		default: 
			ET_FAIL("Unknown texture format");
	}
}

void streamReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
	reinterpret_cast<std::istream*>(png_get_io_ptr(pngPtr))->read((char*)data,
		static_cast<std::streamsize>(length));
}

void handlePngError(png_structp, png_const_charp e)
{
	log::error("[PNGLoader] %s", e);
}

void handlePngWarning(png_structp, png_const_charp)
{
//	supress warnings
//	log::warning("[PNGLoader] %s", e);
}
