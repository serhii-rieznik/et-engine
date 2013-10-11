/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et/core/stream.h>
#include <et/opengl/opengl.h>
#include <et/imaging/jpgloader.h>

#include <libjpeg/jpeglib.h>

using namespace et;

void JPGLoader::loadInfoFromStream(std::istream& stream, TextureDescription& desc)
{
	if (stream.fail()) return;
	
	BinaryDataStorage buffer(streamSize(stream));
	stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.dataSize()));
	
	jpeg_decompress_struct cinfo = { };
	jpeg_error_mgr jerr = { };
	
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, buffer.data(), buffer.dataSize());
	jpeg_read_header(&cinfo, TRUE);
	if (cinfo.out_color_space == JCS_GRAYSCALE)
	{
		jpeg_destroy_decompress(&cinfo);
		return;
	}

	desc.size = vec2i(static_cast<int>(cinfo.image_width), static_cast<int>(cinfo.image_height));
	desc.target = GL_TEXTURE_2D;
	desc.internalformat = GL_RGB;
	desc.format = GL_RGB;
	desc.type = GL_UNSIGNED_BYTE;
	desc.compressed = 0;
	desc.bitsPerPixel = 24;
	desc.channels = 3;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	
	jpeg_destroy_decompress(&cinfo);
}

void JPGLoader::loadFromStream(std::istream& stream, TextureDescription& desc, bool flipped)
{
	if (stream.fail()) return;
	
	BinaryDataStorage buffer(streamSize(stream));
	stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.dataSize()));

	jpeg_decompress_struct cinfo = { };
	jpeg_error_mgr jerr = { };
	
	cinfo.err = jpeg_std_error(&jerr);
	
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, buffer.data(), buffer.dataSize());
	jpeg_read_header(&cinfo, TRUE);
	jpeg_calc_output_dimensions(&cinfo);
	
	if (cinfo.out_color_space == JCS_GRAYSCALE)
	{
		jpeg_destroy_decompress(&cinfo);
		return;
	}
	
	size_t rowSize = static_cast<size_t>(cinfo.output_components * cinfo.output_width);
	
	desc.size = vec2i(static_cast<int>(cinfo.output_width), static_cast<int>(cinfo.output_height));
	desc.target = GL_TEXTURE_2D;
	desc.internalformat = GL_RGB;
	desc.format = GL_RGB;
	desc.type = GL_UNSIGNED_BYTE;
	desc.compressed = 0;
	desc.bitsPerPixel = 24;
	desc.channels = 3;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	desc.data = BinaryDataStorage(rowSize * cinfo.output_height);
	
	if (jpeg_start_decompress(&cinfo))
	{
		if (flipped)
		{
			unsigned char* p_line = desc.data.data() + desc.data.size() - rowSize;
			while (cinfo.output_scanline < cinfo.output_height)
			{
				JDIMENSION linesRead = jpeg_read_scanlines(&cinfo, &p_line, 1);
				p_line -= linesRead * rowSize;
			}
			
		}
		else
		{
			unsigned char* p_line = desc.data.data();
			while (cinfo.output_scanline < cinfo.output_height)
			{
				JDIMENSION linesRead = jpeg_read_scanlines(&cinfo, &p_line, 1);
				p_line += linesRead * rowSize;
			}
		}
	}
	else
	{
		log::warning("Unable to decompress JPEG");
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

void JPGLoader::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream stream(path, StreamMode_Binary);
	if (stream.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(stream.stream(), desc);
	}
}

void JPGLoader::loadFromFile(const std::string& path, TextureDescription& desc, bool flipped)
{
	InputStream stream(path, StreamMode_Binary);
	if (stream.valid())
	{
		desc.setOrigin(path);
		loadFromStream(stream.stream(), desc, flipped);
	}
}
