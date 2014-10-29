/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <libpng/png.h>
#include <et/imaging/imagewriter.h>

using namespace et;

bool internal_writePNGtoFile(const std::string& fileName, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, bool flip);

bool internal_writePNGtoBuffer(BinaryDataStorage& buffer, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, bool flip);

void internal_func_writePNGtoBuffer(png_structp png_ptr, png_bytep data, png_size_t length);
void internal_func_PNGflush(png_structp png_ptr);

bool ImageWriter::writeImageToFile(const std::string& fileName, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip)
{
	switch (fmt)
	{
	case ImageFormat_PNG:
		return internal_writePNGtoFile(fileName, data, size, components, bitsPerComponent, flip);

	default:
		return false;
	}
}

bool ImageWriter::writeImageToBuffer(BinaryDataStorage& buffer, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, ImageFormat fmt, bool flip)
{
	switch (fmt)
	{
		case ImageFormat_PNG:
			return internal_writePNGtoBuffer(buffer, data, size, components, bitsPerComponent, flip);
			
		default:
			return false;
	}
	
}

std::string ImageWriter::extensionForImageFormat(ImageFormat fmt)
{
	switch (fmt)
	{
	case ImageFormat_PNG:
		return ".png";

	default:
		return ".unrecognizedimageformat";
	}
}

void internal_func_writePNGtoBuffer(png_structp png_ptr, png_bytep data, png_size_t length)
{
	BinaryDataStorage* buffer = reinterpret_cast<BinaryDataStorage*>(png_get_io_ptr(png_ptr));

	buffer->fitToSize(length);
	etCopyMemory(buffer->current_ptr(), data, length);
	buffer->applyOffset(length);
}

bool internal_writePNGtoBuffer(BinaryDataStorage& buffer, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, bool flip)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		return false;
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
	
	png_init_io(png_ptr, 0);
	
	png_byte colorType = 0;
	switch (components)
	{
		case 3: 
		{
			colorType = PNG_COLOR_TYPE_RGB;
			break;
		}
		case 4: 
		{
			colorType = PNG_COLOR_TYPE_RGBA;
			break;
		}
	}
	
	png_uint_32 w = static_cast<png_uint_32>(size.x);
	png_uint_32 h = static_cast<png_uint_32>(size.y);
	png_set_IHDR(png_ptr, info_ptr, w, h, bitsPerComponent, colorType,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_bytep* row_pointers = reinterpret_cast<png_bytep*>(sharedObjectFactory().allocator()->alloc(sizeof(png_bytep) * size.y));
	
	int rowSize = size.x * components * bitsPerComponent / 8;
	
	if (flip)
	{
		for (int y = 0; y < size.y; y++)
			row_pointers[y] = (png_bytep)(&data[(size.y - 1 - y) * rowSize]);
	}
	else
	{
		for (int y = 0; y < size.y; y++)
			row_pointers[y] = (png_bytep)(&data[y * rowSize]);
	}
	
	png_set_write_fn(png_ptr, &buffer, internal_func_writePNGtoBuffer, 0);
	
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, NULL);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);
	
	sharedObjectFactory().allocator()->free(row_pointers);
	return true;
}

bool internal_writePNGtoFile(const std::string& fileName, const BinaryDataStorage& data,
	const vec2i& size, int components, int bitsPerComponent, bool flip)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr)
		return false;
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
	
	FILE* fp = fopen(fileName.c_str(), "wb");
	if (!fp) 
		return false; 
	
	png_init_io(png_ptr, fp);
	
	png_byte colorType = 0;
	switch (components)
	{
		case 1:
		{
			colorType = PNG_COLOR_TYPE_GRAY;
			break;
		}
			
		case 3: 
		{
			colorType = PNG_COLOR_TYPE_RGB;
			break;
		}
		case 4: 
		{
			colorType = PNG_COLOR_TYPE_RGBA;
			break;
		}
			
		default:
			ET_FAIL("Invalid components number.");
	}
	
	png_uint_32 w = static_cast<png_uint_32>(size.x);
	png_uint_32 h = static_cast<png_uint_32>(size.y);
	int rowSize = size.x * components * bitsPerComponent / 8;
	
	png_set_IHDR(png_ptr, info_ptr, w, h, bitsPerComponent, colorType,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_write_info(png_ptr, info_ptr);

	png_set_compression_level(png_ptr, 3);
	png_set_compression_strategy(png_ptr, 0);
	png_set_filter(png_ptr, 0, PNG_NO_FILTERS);
	
	if (flip)
	{
		for (int y = 0; y < size.y; y++)
			png_write_row(png_ptr, (png_bytep)(&data[(size.y - 1 - y) * rowSize]));
	}
	else
	{
		for (int y = 0; y < size.y; y++)
			png_write_row(png_ptr, (png_bytep)(&data[y * rowSize]));
	}
	
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	
	return true;
}
