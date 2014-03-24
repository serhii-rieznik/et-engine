/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/core/tools.h>
#include <et/opengl/opengl.h>
#include <et/threading/threading.h>

using namespace et;

#define ET_STOP_ON_OPENGL_ERROR			1
#define CASE_VALUE(V)					case V: return #V;

size_t OpenGLCounters::primitiveCounter = 0;
size_t OpenGLCounters::DIPCounter = 0;
size_t OpenGLCounters::bindTextureCounter = 0;
size_t OpenGLCounters::bindBufferCounter = 0;
size_t OpenGLCounters::bindFramebufferCounter = 0;
size_t OpenGLCounters::useProgramCounter = 0;
size_t OpenGLCounters::bindVertexArrayObjectCounter = 0;

void OpenGLCounters::reset()
{
	primitiveCounter = 0;
	DIPCounter = 0;
	bindTextureCounter = 0;
	bindBufferCounter = 0;
	bindFramebufferCounter = 0;
	useProgramCounter = 0;
	bindVertexArrayObjectCounter = 0;
}

OpenGLDebugScope::OpenGLDebugScope(const std::string& info)
{
#if defined(GL_EXT_debug_marker)
	glPushGroupMarkerEXT(static_cast<GLsizei>(info.size()), info.c_str());
#endif
}

OpenGLDebugScope::~OpenGLDebugScope()
{
#if defined(GL_EXT_debug_marker)
	glPopGroupMarkerEXT();
#endif
}

std::string et::glErrorToString(uint32_t error)
{
	switch (error)
	{
		CASE_VALUE(GL_NO_ERROR)
		CASE_VALUE(GL_INVALID_ENUM)
		CASE_VALUE(GL_INVALID_VALUE)
		CASE_VALUE(GL_INVALID_OPERATION)
		CASE_VALUE(GL_OUT_OF_MEMORY)
		CASE_VALUE(GL_INVALID_FRAMEBUFFER_OPERATION)

	default:
		return "Unknown OpenGL error " + intToStr(error);
	}
}

void et::checkOpenGLErrorEx(const char* caller, const char* fileName, const char* line, const char* tag, ...)
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		char buffer[1024] = { };
		
		va_list args;
		va_start(args, tag);
		vsnprintf(buffer, sizeof(buffer), tag, args);
		va_end(args);
		
		log::error("[%s:%s] %s\n{\n\ttag = %s\n\terr = %s\n}\n", fileName, line, caller,
			buffer, glErrorToString(error).c_str());

#if (ET_STOP_ON_OPENGL_ERROR)
		abort();
#endif
	}
}

size_t et::primitiveCount(uint32_t mode, size_t count)
{
	switch (mode)
	{
	case GL_TRIANGLES:
		return count / 3;

	case GL_TRIANGLE_STRIP:
		return count - 2;

	case GL_LINES:
		return count / 2;

	case GL_LINE_STRIP:
		return count - 1;

	case GL_POINTS:
		return count;

	default:
		return 0;
	};
}

std::string et::glBlendFuncToString(uint32_t value)
{
	switch (value)
	{
		CASE_VALUE(GL_ZERO)
		CASE_VALUE(GL_ONE)
		CASE_VALUE(GL_SRC_COLOR)
		CASE_VALUE(GL_ONE_MINUS_SRC_COLOR)
		CASE_VALUE(GL_SRC_ALPHA)
		CASE_VALUE(GL_ONE_MINUS_SRC_ALPHA)
		CASE_VALUE(GL_DST_ALPHA)
		CASE_VALUE(GL_ONE_MINUS_DST_ALPHA)
		CASE_VALUE(GL_DST_COLOR)
		CASE_VALUE(GL_ONE_MINUS_DST_COLOR)
		CASE_VALUE(GL_SRC_ALPHA_SATURATE)
			
		default:
			return "Unknown blend function " + intToStr(value);
	}
}

std::string et::glTexTargetToString(uint32_t target)
{
	switch (target)
	{
		CASE_VALUE(GL_TEXTURE_2D)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_POSITIVE_X)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_NEGATIVE_X)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_POSITIVE_Z)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
			
#if defined(GL_TEXTURE_1D)
		CASE_VALUE(GL_TEXTURE_1D)
#endif
#if defined(GL_TEXTURE_3D)
		CASE_VALUE(GL_TEXTURE_3D)
#endif
#if defined(GL_TEXTURE_2D_ARRAY)
		CASE_VALUE(GL_TEXTURE_2D_ARRAY)
#endif
#if defined(GL_TEXTURE_CUBE_MAP_ARRAY)
		CASE_VALUE(GL_TEXTURE_CUBE_MAP_ARRAY)
#endif
#if defined(GL_TEXTURE_RECTANGLE)
		CASE_VALUE(GL_TEXTURE_RECTANGLE)
#endif
#if defined(GL_TEXTURE_2D_MULTISAMPLE)
		CASE_VALUE(GL_TEXTURE_2D_MULTISAMPLE)
#endif
#if defined(GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
		CASE_VALUE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
#endif
#if defined(GL_TEXTURE_BUFFER)
		CASE_VALUE(GL_TEXTURE_BUFFER)
#endif
		default:
			return "Unknown texture target " + intToStr(target);
	}
}

std::string et::glInternalFormatToString(int format)
{
	switch (format)
	{
		CASE_VALUE(GL_DEPTH_COMPONENT)
		CASE_VALUE(GL_DEPTH_COMPONENT16)
		
		CASE_VALUE(GL_RGB)
		CASE_VALUE(GL_RGBA)
		CASE_VALUE(GL_RGBA4)
		CASE_VALUE(GL_RGB5_A1)
		
#if defined(GL_RG32F)
		CASE_VALUE(GL_RG32F)
#endif
		
#if defined(GL_RGB32F)
		CASE_VALUE(GL_RGB32F)
#endif
		
#if defined(GL_RGBA32F)
		CASE_VALUE(GL_RGBA32F)
#endif
		
#if defined(GL_RGB4)
		CASE_VALUE(GL_RGB4)
#endif
		
#if defined(GL_RGB565)
		CASE_VALUE(GL_RGB565)
#endif
		
#if defined(GL_DEPTH_COMPONENT24)
		CASE_VALUE(GL_DEPTH_COMPONENT24)
#endif
		
#if defined(GL_BGRA)
		CASE_VALUE(GL_BGRA)
#endif
		
#if defined (GL_LUMINANCE)
		CASE_VALUE(GL_LUMINANCE)
#endif
		
#if defined (GL_LUMINANCE_ALPHA)
		CASE_VALUE(GL_LUMINANCE_ALPHA)
#endif
		
#if defined(GL_RGB8) && (GL_RGB8 != GL_RGB)
		CASE_VALUE(GL_RGB8)
#endif
		
#if defined(GL_RGBA8) && (GL_RGBA8 != GL_RGBA)
		CASE_VALUE(GL_RGBA8)
#endif
		
#if defined(GL_INTENSITY)
		CASE_VALUE(GL_INTENSITY)
		CASE_VALUE(GL_INTENSITY8)
		CASE_VALUE(GL_INTENSITY16)
		CASE_VALUE(GL_LUMINANCE8)
		CASE_VALUE(GL_LUMINANCE16)
#endif
		
#if defined(GL_RGB16F)
		CASE_VALUE(GL_RGB16F)
#endif
		
#if defined(GL_RGBA16F)
		CASE_VALUE(GL_RGBA16F)
#endif
		
		
#if defined(GL_R11F_G11F_B10F)
		CASE_VALUE(GL_R11F_G11F_B10F)
#endif
		
#if defined(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
		CASE_VALUE(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
#endif
		
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		CASE_VALUE(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
#endif
		
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
		CASE_VALUE(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
#endif
		
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		CASE_VALUE(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
#endif
		
#if defined(GL_COMPRESSED_RG_RGTC2)
		CASE_VALUE(GL_COMPRESSED_RG_RGTC2)
#endif
		
#if defined(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
		CASE_VALUE(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
		CASE_VALUE(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG)
		CASE_VALUE(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
		CASE_VALUE(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
#endif

	default:
		return "Unknown texture format #" + intToStr(format);
	}
}

std::string et::glTypeToString(uint32_t type)
{
	switch (type)
	{
		CASE_VALUE(GL_UNSIGNED_BYTE)
		CASE_VALUE(GL_BYTE)
		CASE_VALUE(GL_UNSIGNED_SHORT)
		CASE_VALUE(GL_SHORT)
		CASE_VALUE(GL_UNSIGNED_INT)
		CASE_VALUE(GL_INT)
		CASE_VALUE(GL_FLOAT)

#if defined(GL_HALF_FLOAT)
		CASE_VALUE(GL_HALF_FLOAT)
#endif
		default:
			return "Unknown type " + intToStr(type);
	}
}

std::string et::glPrimitiveTypeToString(uint32_t value)
{
	switch (value)
	{
		CASE_VALUE(GL_POINTS)
		CASE_VALUE(GL_LINES)
		CASE_VALUE(GL_LINE_LOOP)
		CASE_VALUE(GL_LINE_STRIP)
		CASE_VALUE(GL_TRIANGLES)
		CASE_VALUE(GL_TRIANGLE_STRIP)
		CASE_VALUE(GL_TRIANGLE_FAN)
		default:
			return "Unknown primitive type " + intToStr(value);
	}
}

void et::etDrawElementsInstanced(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices,
	GLsizei instanceCount)
{
	glDrawElementsInstanced(mode, count, type, indices, instanceCount);
	checkOpenGLError("glDrawElementsInstanced(%d, %d, %u, %08x, %d)", mode, count, type, indices, instanceCount);
	
#	if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += instanceCount * primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#	endif
}

#if defined(GL_ARB_draw_elements_base_vertex)
void et::etDrawElementsBaseVertex(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices, int base)
{
	glDrawElementsBaseVertex(mode, count, type, indices, base);
	checkOpenGLError("glDrawElementsBaseVertex(mode, count, type, indices, base)");

#	if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#	endif
}
#else
void et::etDrawElementsBaseVertex(uint32_t, GLsizei, uint32_t, const GLvoid*, int)
{
	log::warning("Call to glDrawElementsBaseVertex without defined GL_ARB_draw_elements_base_vertex");
}
#endif

void et::etDrawElements(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices)
{
	glDrawElements(mode, count, type, indices);
	
	checkOpenGLError("glDrawElements(%s, %u, %s, 0x%08X)", glPrimitiveTypeToString(mode).c_str(),
		count, glTypeToString(type).c_str(), indices);

#if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#endif
}

void et::etBindTexture(uint32_t target, uint32_t texture)
{
	if (texture == 0)
		texture = 0;
	
	glBindTexture(target, texture);
	checkOpenGLError("glBindTexture(%u, %d)", target, texture);

#if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindTextureCounter;
#endif
}

void et::etBindBuffer(uint32_t target, uint32_t buffer)
{
	glBindBuffer(target, buffer);
	checkOpenGLError("glBindBuffer(%u, %u)", target, buffer);

#if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindBufferCounter;
#endif
}

void et::etBindFramebuffer(uint32_t target, uint32_t framebuffer)
{
	glBindFramebuffer(target, framebuffer);
	checkOpenGLError("glBindFramebuffer(%u, %u)", target, framebuffer);

#if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindFramebufferCounter;
#endif
}

void et::etViewport(int x, int y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
	checkOpenGLError("glViewport(%d, %d, %u, %u)", x, y, width, height);
}

void et::etUseProgram(uint32_t program)
{
	glUseProgram(program);
	checkOpenGLError("glUseProgram(%u)", program);

#if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::useProgramCounter;
#endif
}

#if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
void et::etBindVertexArray(uint32_t arr)
{
	glBindVertexArray(arr);
	checkOpenGLError("glBindVertexArray(%u)", arr);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindVertexArrayObjectCounter;
#	endif
}
#else
void et::etBindVertexArray(uint32_t)
{
	log::warning("Call to glBindVertexArray without defined "
		"GL_ARB_vertex_array_object or GL_OES_vertex_array_object");
}
#endif

int32_t et::textureWrapValue(TextureWrap w)
{
	switch (w)
	{
		case TextureWrap_Repeat:
			return GL_REPEAT;
		case TextureWrap_ClampToEdge:
			return GL_CLAMP_TO_EDGE;
		case TextureWrap_MirrorRepeat:
			return GL_MIRRORED_REPEAT;
		default:
			ET_ASSERT(false && "Unrecognized texture wrap.");
	}
	
	return 0;
}

int32_t et::textureFiltrationValue(TextureFiltration f)
{
	switch (f)
	{
		case TextureFiltration_Nearest:
			return GL_NEAREST;
		case TextureFiltration_Linear:
			return GL_LINEAR;
		case TextureFiltration_NearestMipMapNearest:
			return GL_NEAREST_MIPMAP_NEAREST;
		case TextureFiltration_NearestMipMapLinear:
			return GL_NEAREST_MIPMAP_LINEAR;
		case TextureFiltration_LinearMipMapNearest:
			return GL_LINEAR_MIPMAP_NEAREST;
		case TextureFiltration_LinearMipMapLinear:
			return GL_LINEAR_MIPMAP_LINEAR;
		default:
			ET_ASSERT(false && "Unrecognized texture filtration.");
	}
	
	return 0;
}

uint32_t et::drawTypeValue(BufferDrawType t)
{
	switch (t)
	{
		case BufferDrawType_Static:
			return GL_STATIC_DRAW;
		case BufferDrawType_Dynamic:
			return GL_DYNAMIC_DRAW;
		case BufferDrawType_Stream:
			return GL_STREAM_DRAW;
		default:
			ET_ASSERT(false && "Unrecognized draw type");
	}
	
	return 0;
}

uint32_t et::primitiveTypeValue(PrimitiveType t)
{
	switch (t)
	{
		case PrimitiveType_Points:
			return GL_POINTS;
		case PrimitiveType_Lines:
			return GL_LINES;
		case PrimitiveType_LineStrip:
			return GL_LINE_STRIP;
		case PrimitiveType_Triangles:
			return GL_TRIANGLES;
		case PrimitiveType_TriangleStrips:
			return GL_TRIANGLE_STRIP;
		default:
			ET_ASSERT(false && "Invalid PrimitiveType value");
	}
	
	return 0;
}

#if (ET_OPENGLES)

void et::etTexImage1D(uint32_t, int, int, GLsizei, int, uint32_t, uint32_t, const GLvoid*)
	{ ET_FAIL("Call to glTexImage1D in OpenGL ES"); }

void et::etCompressedTexImage1D(uint32_t, int, uint32_t, GLsizei, int, GLsizei, const GLvoid*)
	{ ET_FAIL("Call to glCompressedTexImage1D in OpenGL ES"); }

#else

void et::etTexImage1D(uint32_t target, int level, int internalformat, GLsizei width, int border,
	uint32_t format, uint32_t type, const GLvoid * pixels)
{
	glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
	
	checkOpenGLError("glTexImage2D(%s, %d, %s, %d, %d, %s, %s, %, 0x%8X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(internalformat).c_str(),
		width, border, glInternalFormatToString(static_cast<int32_t>(format)).c_str(), glTypeToString(type).c_str(), pixels);
}

void et::etCompressedTexImage1D(uint32_t target, int level, uint32_t internalformat,
	GLsizei width, int border, GLsizei imageSize, const GLvoid * data)
{
	glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
	checkOpenGLError("glCompressedTexImage1D(%s, %d, %s, %d, %d, %d, 0x%8X)", glTexTargetToString(target).c_str(),
		level, glInternalFormatToString(static_cast<int32_t>(internalformat)).c_str(),width, border, imageSize, data);
}

#endif

void et::etCompressedTexImage2D(uint32_t target, int level, uint32_t internalformat,
	GLsizei width, GLsizei height, int border, GLsizei imageSize, const GLvoid * data)
{
	glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);

#if (ET_DEBUG)
	checkOpenGLError("glCompressedTexImage2D(%s, %d, %s, %d, %d, %d, %d, 0x%8X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(static_cast<int32_t>(internalformat)).c_str(),
		width, height, border, imageSize, data);
#endif
}

void et::etTexImage2D(uint32_t target, int level, int internalformat, GLsizei width, GLsizei height,
	int border, uint32_t format, uint32_t type, const GLvoid* pixels)
{
	ET_ASSERT(pixels);
	
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);

#if (ET_DEBUG)
	checkOpenGLError("glTexImage2D(%s, %d, %s, %d, %d, %d, %s, %s, %, 0x%08X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(internalformat).c_str(),
		width, height, border, glInternalFormatToString(static_cast<int32_t>(format)).c_str(), glTypeToString(type).c_str(),
		pixels);
#endif
}

#if (ET_PLATFORM_WIN)
#	define ET_VALIDATE_GLFUNC_EXT(F) if (F == nullptr) F = F##EXT;
#else
#	define ET_VALIDATE_GLFUNC_EXT(F)
#endif

void et::validateExtensions()
{
	ET_VALIDATE_GLFUNC_EXT(glIsRenderbuffer);
	ET_VALIDATE_GLFUNC_EXT(glBindRenderbuffer);
	ET_VALIDATE_GLFUNC_EXT(glDeleteRenderbuffers);
	ET_VALIDATE_GLFUNC_EXT(glGenRenderbuffers);
	ET_VALIDATE_GLFUNC_EXT(glRenderbufferStorage);
	ET_VALIDATE_GLFUNC_EXT(glGetRenderbufferParameteriv);
	ET_VALIDATE_GLFUNC_EXT(glIsFramebuffer);
	ET_VALIDATE_GLFUNC_EXT(glBindFramebuffer);
	ET_VALIDATE_GLFUNC_EXT(glDeleteFramebuffers);
	ET_VALIDATE_GLFUNC_EXT(glGenFramebuffers);
	ET_VALIDATE_GLFUNC_EXT(glCheckFramebufferStatus);
	ET_VALIDATE_GLFUNC_EXT(glFramebufferTexture1D);
	ET_VALIDATE_GLFUNC_EXT(glFramebufferTexture2D);
	ET_VALIDATE_GLFUNC_EXT(glFramebufferTexture3D);
	ET_VALIDATE_GLFUNC_EXT(glFramebufferRenderbuffer);
	ET_VALIDATE_GLFUNC_EXT(glGetFramebufferAttachmentParameteriv);
	ET_VALIDATE_GLFUNC_EXT(glGenerateMipmap);
	ET_VALIDATE_GLFUNC_EXT(glBlitFramebuffer);
	ET_VALIDATE_GLFUNC_EXT(glRenderbufferStorageMultisample);
}

size_t et::bitsPerPixelForType(uint32_t type)
{
	switch (type)
	{
		case GL_UNSIGNED_BYTE:
		case GL_BYTE:
			return 8;
			
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
			return 16;

		case GL_UNSIGNED_INT:
		case GL_INT:
			return 32;
			
		case GL_FLOAT:
			return 32;
			
		case GL_HALF_FLOAT:
			return 16;
			
		default:
			ET_FAIL("Not yet implemented for this type.");
			return 0;
	}
}

size_t et::bitsPerPixelForTextureFormat(uint32_t internalFormat, uint32_t type)
{
	switch (internalFormat)
	{
		case GL_RGB:
		{
			switch (type)
			{
				case GL_UNSIGNED_SHORT_5_6_5:
					return bitsPerPixelForType(type);
					
				default:
					return 3 * bitsPerPixelForType(type);
			}
		}
			
		case GL_RGBA:
		{
			switch (type)
			{
				case GL_UNSIGNED_SHORT_5_5_5_1:
				case GL_UNSIGNED_SHORT_4_4_4_4:
					return bitsPerPixelForType(type);
					
				default:
					return 4 * bitsPerPixelForType(type);
			}
		}
			
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
			return bitsPerPixelForType(type);

		case GL_RGB32F:
			return 3 * bitsPerPixelForType(type);
			
		case GL_RGBA32F:
			return 4 * bitsPerPixelForType(type);

#if defined(GL_RED)
		case GL_RED:
			return bitsPerPixelForType(type);
#endif
			
#if defined(GL_RG)
		case GL_RG:
			return 2 * bitsPerPixelForType(type);
#endif
			
#if defined(GL_RG16F)
		case GL_RG16F:
			return 2 * bitsPerPixelForType(type);
#endif
			
#if defined(GL_RGB16F)
		case GL_RGB16F:
			return 3 * bitsPerPixelForType(type);
#endif
			
#if defined(GL_RGBA16F)
		case GL_RGBA16F:
			return 4 * bitsPerPixelForType(type);
#endif
			
#if defined(GL_R32F)
		case GL_R32F:
			return bitsPerPixelForType(type);
#endif

#if defined(GL_RG32F)
		case GL_RG32F:
			return 2 * bitsPerPixelForType(type);
#endif
			
#if defined(GL_RGB565)
		case GL_RGB565:
			return 2;
#endif

#if defined(GL_LUMINANCE)
		case GL_LUMINANCE:
			return bitsPerPixelForType(type);
#endif

#if defined(GL_LUMINANCE_ALPHA)
		case GL_LUMINANCE_ALPHA:
			return bitsPerPixelForType(type);
#endif
			
		default:
			ET_FAIL("Not yet implemented for this format");
			return 0;
	}
}
