/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/imaging/jpegloader.h>
#include <libjpeg/jpeglib.h>
#include <setjmp.h>

using namespace et;

const size_t jpegBufferSize = 10240;

struct jpegErrorManager
{
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

struct jpegStreamWrapper
{
	jpeg_source_mgr pub;
	std::istream* stream;
	unsigned char buffer[jpegBufferSize];
};

void skip(j_decompress_ptr cinfo, long count);
boolean fill_buffer(j_decompress_ptr cinfo);
void init_source(j_decompress_ptr cinfo);
void my_error_exit(j_common_ptr cinfo);
void term(j_decompress_ptr cinfo);

void et::jpeg::loadInfoFromStream(std::istream& source, TextureDescription& desc)
{
	jpeg_decompress_struct cinfo = { };
	
	jpegErrorManager errorManager = { };
	cinfo.err = jpeg_std_error(&errorManager.pub);
	errorManager.pub.error_exit = my_error_exit;
	
	if (setjmp(errorManager.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return;
	}
	
	jpegStreamWrapper* streamWrapper = nullptr;
	
	jpeg_create_decompress(&cinfo);
	
	cinfo.src = (jpeg_source_mgr*)(cinfo.mem->alloc_small)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_PERMANENT, sizeof(jpegStreamWrapper));
	streamWrapper = reinterpret_cast<jpegStreamWrapper*>(cinfo.src);
	streamWrapper->pub.init_source = init_source;
    streamWrapper->pub.fill_input_buffer = fill_buffer;
    streamWrapper->pub.skip_input_data = skip;
    streamWrapper->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    streamWrapper->pub.term_source = term;
    streamWrapper->stream = &source;
    streamWrapper->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    streamWrapper->pub.next_input_byte = nullptr; /* until buffer loaded */
	
	jpeg_read_header(&cinfo, TRUE);
	
	desc.size = vec2i(cinfo.output_width, cinfo.output_height);
	desc.bitsPerPixel = 8 * cinfo.output_components;
	desc.channels = cinfo.output_components;
	
	switch (cinfo.out_color_space)
	{
		case JCS_GRAYSCALE:
		{
#		if defined(GL_R8)
			desc.internalformat = GL_R8;
			desc.format = GL_RED;
#		else
			desc.internalformat = GL_LUMINANCE;
			desc.format = GL_LUMINANCE;
#		endif
			break;
		}
			
		case JCS_CMYK:
		{
			desc.internalformat = GL_RGBA;
			desc.format = GL_RGBA;
			break;
		}
			
		default:
		{
			desc.internalformat = GL_RGB;
			desc.format = GL_RGB;
			break;
		}
	}
	
	desc.type = GL_UNSIGNED_BYTE;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	
	jpeg_destroy_decompress(&cinfo);
}

void et::jpeg::loadFromStream(std::istream& source, TextureDescription& desc)
{
	jpeg_decompress_struct cinfo = { };
	
	jpegErrorManager errorManager = { };
	cinfo.err = jpeg_std_error(&errorManager.pub);
	errorManager.pub.error_exit = my_error_exit;
	
	if (setjmp(errorManager.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return;
	}
	
	jpegStreamWrapper* streamWrapper = nullptr;
	
	jpeg_create_decompress(&cinfo);
	
	cinfo.src = (jpeg_source_mgr*)(cinfo.mem->alloc_small)(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_PERMANENT, sizeof(jpegStreamWrapper));
	streamWrapper = reinterpret_cast<jpegStreamWrapper*>(cinfo.src);
	streamWrapper->pub.init_source = init_source;
    streamWrapper->pub.fill_input_buffer = fill_buffer;
    streamWrapper->pub.skip_input_data = skip;
    streamWrapper->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    streamWrapper->pub.term_source = term;
    streamWrapper->stream = &source;
    streamWrapper->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    streamWrapper->pub.next_input_byte = nullptr; /* until buffer loaded */
	
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	
	desc.size = vec2i(cinfo.output_width, cinfo.output_height);
	desc.bitsPerPixel = 8 * cinfo.output_components;
	desc.channels = cinfo.output_components;
	
	switch (cinfo.out_color_space)
	{
		case JCS_GRAYSCALE:
		{
#		if defined(GL_R8)
			desc.internalformat = GL_R8;
			desc.format = GL_RED;
#		else
			desc.internalformat = GL_LUMINANCE;
			desc.format = GL_LUMINANCE;
#		endif
			break;
		}
			
		case JCS_CMYK:
		{
			desc.internalformat = GL_RGBA;
			desc.format = GL_RGBA;
			break;
		}
			
		default:
		{
			desc.internalformat = GL_RGB;
			desc.format = GL_RGB;
			break;
		}
	}
	
	desc.type = GL_UNSIGNED_BYTE;
	desc.mipMapCount = 1;
	desc.layersCount = 1;
	
	auto row_stride = cinfo.output_width * cinfo.output_components;
	desc.data.resize(row_stride * cinfo.output_height);
	
	do
	{
		unsigned char* position = desc.data.element_ptr((cinfo.output_height - cinfo.output_scanline - 1) * row_stride);
		JSAMPARRAY scanlines = &position;
		jpeg_read_scanlines(&cinfo, scanlines, 1);
	}
	while (cinfo.output_scanline < cinfo.output_height);
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

void et::jpeg::loadFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadFromStream(file.stream(), desc);
	}
}

void et::jpeg::loadInfoFromFile(const std::string& path, TextureDescription& desc)
{
	InputStream file(path, StreamMode_Binary);
	if (file.valid())
	{
		desc.setOrigin(path);
		loadInfoFromStream(file.stream(), desc);
	}
}

/*
 * Service functions
 */
void my_error_exit(j_common_ptr cinfo)
{
	jpegErrorManager* myerr = reinterpret_cast<jpegErrorManager*>(cinfo->err);
	(*cinfo->err->output_message)(cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

void init_source(j_decompress_ptr)
{
}

boolean fill_buffer(j_decompress_ptr cinfo)
{
    auto src = (jpegStreamWrapper*)(cinfo->src);
	src->stream->read(reinterpret_cast<char*>(src->buffer), jpegBufferSize);
    src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = static_cast<size_t>(src->stream->gcount());
    return TRUE;
}

void skip(j_decompress_ptr cinfo, long count)
{
    auto src = (jpegStreamWrapper*)(cinfo->src);
	
	long bytesToAdjust = etMin(static_cast<long>(src->pub.bytes_in_buffer), count);
	long bytesToSkip = etMax(0l, count - bytesToAdjust);
	
	if (bytesToAdjust > 0)
	{
		src->pub.next_input_byte += bytesToAdjust;
		src->pub.bytes_in_buffer -= bytesToAdjust;
	}
	
	if (bytesToSkip > 0)
		src->stream->seekg(bytesToSkip, std::ios::cur);
}

void term(j_decompress_ptr)
{
}
