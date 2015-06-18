/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
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
#if defined(GL_EXT_debug_marker) && !defined(ET_CONSOLE_APPLICATION)
	glPushGroupMarkerEXT(static_cast<GLsizei>(info.size()), info.c_str());
#else
	(void)info;
#endif
}

OpenGLDebugScope::~OpenGLDebugScope()
{
#if defined(GL_EXT_debug_marker) && !defined(ET_CONSOLE_APPLICATION)
	glPopGroupMarkerEXT();
#endif
}

VertexAttributeType et::openglTypeToVertexAttributeType(uint32_t value)
{
	switch (value)
	{
		case GL_FLOAT:
			return VertexAttributeType::Float;
			
		case GL_FLOAT_VEC2:
			return VertexAttributeType::Vec2;
			
		case GL_FLOAT_VEC3:
			return VertexAttributeType::Vec3;
			
		case GL_FLOAT_VEC4:
			return VertexAttributeType::Vec4;
			
		case GL_FLOAT_MAT3:
			return VertexAttributeType::Mat3;
			
		case GL_FLOAT_MAT4:
			return VertexAttributeType::Mat4;
			
		case GL_INT:
			return VertexAttributeType::Int;
			
		default:
			ET_FAIL_FMT("Unsupported OpenGL type %u (0x%X)", value, value);
	}
}

#define ET_SAMPLE_VALUE_FROM_MAP 	{ \
										uint32_t intValue = static_cast<uint32_t>(value); \
										ET_ASSERT(valuesMap[intValue] != 0); \
										return valuesMap[intValue]; \
									}

uint32_t et::vertexAttributeTypeValue(VertexAttributeType value)
{
	ET_ASSERT(value < VertexAttributeType::max)
	
	static const uint32_t valuesMap[VertexAttributeType_max] =
	{
		GL_FLOAT,
		GL_FLOAT_VEC2,
		GL_FLOAT_VEC3,
		GL_FLOAT_VEC4,
		GL_FLOAT_MAT3,
		GL_FLOAT_MAT4,
		GL_INT
	};
	ET_SAMPLE_VALUE_FROM_MAP
}

static const std::pair<uint32_t, uint32_t> blendStatesMap[BlendState_max] =
{
	{ GL_ONE, GL_ZERO }, // Disabled,
	{ GL_ONE, GL_ZERO }, // Current,
	{ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA }, // Default,
	{ GL_ONE, GL_ONE_MINUS_SRC_ALPHA}, // AlphaPremultiplied,
	{ GL_ONE, GL_ONE }, // Additive,
	{ GL_SRC_ALPHA, GL_ONE }, // AlphaAdditive,
	{ GL_ZERO, GL_SRC_ALPHA }, // AlphaMultiplicative,
	{ GL_SRC_COLOR, GL_ONE}, // ColorAdditive,
	{ GL_ZERO, GL_ONE_MINUS_SRC_ALPHA } // AlphaInverseMultiplicative
};

std::pair<uint32_t, uint32_t> et::blendStateValue(BlendState value)
{
	ET_ASSERT(value < BlendState::max)
	return blendStatesMap[static_cast<uint32_t>(value)];
}

BlendState et::blendValuesToBlendState(uint32_t source, uint32_t dest)
{
	size_t availableBlendModes = sizeof(blendStatesMap) / sizeof(blendStatesMap[0]);
	
	for (size_t i = 0; i < availableBlendModes; ++i)
	{
		if ((blendStatesMap[i].first == source) && (blendStatesMap[i].second == dest))
			return static_cast<BlendState>(i);
	}
	
	log::warning("Unsupported blend combination: %d, %d", source, dest);
	return BlendState::Disabled;
}

uint32_t et::textureFormatValue(TextureFormat value)
{
	ET_ASSERT(value < TextureFormat::max)
	
	static const uint32_t valuesMap[TextureFormat_max] =
	{
		0, // Invalid,
		
		GL_RED, // R,
		
		GL_R8, //R8,
		
#	if defined(GL_RG16)
		GL_R16, //R16,
#	else
		0,
#	endif

		GL_R16F, //R16F,
		
		GL_R32F, //R32F,
		
		GL_RG, //RG,
		
		GL_RG8, //RG8,
		
#	if defined(GL_RG16)
		GL_RG16, //RG16,
#	else
		0,
#	endif

		GL_RG16F, //RG16F,
		
		GL_RG32F, //RG32F,
		
		GL_RGB, //RGB,
		
		GL_RGB8, //RGB8,
		
#	if defined(GL_RGBA16)
		GL_RGB16, //RGB16,
#	else
		0,
#	endif
		
		GL_RGB16F, //RGB16F,
		
		GL_RGB32F, //RGB32F,
		
#	if defined(GL_BGR)
		GL_BGR, //BGR,
#	else
		GL_RGB, // for BGR,
#	endif
		
		GL_RGBA, //RGBA,
		
		GL_RGBA8, //RGBA8,
		
#	if defined(GL_RGBA16)
		GL_RGBA16, //RGBA16,
#	else
		0,
#	endif
		
		GL_RGBA16F, //RGBA16F,
		
		GL_RGBA32F, //RGBA32F,
		
		GL_BGRA, //BGRA,
		
#	if defined(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT, //DXT1_RGB,
#	else
		0,
#	endif
		
#	if defined(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, //DXT1_RGBA,
#	else
		0,
#	endif
		
#	if defined(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
		GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, //DXT3,
#	else
		0,
#	endif
		
#	if defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, //DXT5,
#	else
		0,
#	endif
		
#	if defined(GL_COMPRESSED_RG_RGTC2)
		GL_COMPRESSED_RG_RGTC2, //RGTC2,
#	else
		0,
#	endif
		
		GL_DEPTH_COMPONENT, //Depth,
		GL_DEPTH_COMPONENT16, //Depth16,
		GL_DEPTH_COMPONENT24, //Depth24,
		
#	if defined(GL_DEPTH_COMPONENT32)
		GL_DEPTH_COMPONENT32, //Depth32,
#	else
		0,
#	endif
		
#	if defined(GL_DEPTH_COMPONENT32F)
		GL_DEPTH_COMPONENT32F, //Depth32F,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
		GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, // PVR_2bpp_RGB,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT)
		GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT, // PVR_2bpp_sRGB,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
		GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, // PVR_2bpp_RGBA,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT)
		GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT, // PVR_2bpp_sRGBA,
#	else
		0,
#	endif
		
#	if defined(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG)
		GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, // PVR_4bpp_RGB,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT)
		GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT, // PVR_4bpp_sRGB,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
		GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, // PVR_4bpp_RGBA,
#	else
		0,
#	endif

#	if defined(GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT)
		GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT, // PVR_4bpp_sRGBA,
#	else
		0,
#	endif

#if defined(GL_R11F_G11F_B10F)
		GL_R11F_G11F_B10F
#	else
		0
#	endif
	};

	ET_SAMPLE_VALUE_FROM_MAP
}

uint32_t et::textureTargetValue(TextureTarget value)
{
	static const uint32_t valuesMap[TextureTarget_max] =
	{
		GL_TEXTURE_2D, // Texture_2D,
		
#	if defined(GL_TEXTURE_2D_ARRAY)
		GL_TEXTURE_2D_ARRAY, // Texture_2D_Array,
#	else
		GL_TEXTURE_2D, // Texture_2D_Array,
#	endif
		
#	if defined(GL_TEXTURE_RECTANGLE)
		GL_TEXTURE_RECTANGLE, // Texture_2D_Rect,
#	else
		GL_TEXTURE_2D, // Texture_2D_Rect,
#	endif
		
		GL_TEXTURE_CUBE_MAP, // Texture_Cube,
	};
	
	ET_SAMPLE_VALUE_FROM_MAP
}

uint32_t et::dataTypeValue(DataType value)
{
	ET_ASSERT(value < DataType::max);

	static const uint32_t valuesMap[DataType_max] =
	{
		GL_BYTE, // Char
		GL_UNSIGNED_BYTE, // UnsignedChar
		GL_SHORT, // Short
		GL_UNSIGNED_SHORT, // UnsignedShort
		GL_INT, // Int
		GL_UNSIGNED_INT, // UnsignedInt
		GL_HALF_FLOAT, // Half
		GL_FLOAT, // Float
		
#	if defined(GL_DOUBLE)
		GL_DOUBLE, // Double
#	else
		GL_FLOAT, // Float
#	endif
		
		GL_UNSIGNED_SHORT_4_4_4_4, // UnsignedShort_4444
		GL_UNSIGNED_SHORT_5_5_5_1, // UnsignedShort_5551
		GL_UNSIGNED_SHORT_5_6_5, // UnsignedShort_565

#	if defined(GL_UNSIGNED_INT_8_8_8_8_REV)
		GL_UNSIGNED_INT_8_8_8_8_REV, // UnsignedInt_8888_Rev
#	else
		GL_UNSIGNED_BYTE
#	endif
	};
	
	ET_SAMPLE_VALUE_FROM_MAP
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
#if defined(ET_CONSOLE_APPLICATION)
	
	log::error("Call to OpenGL in console application.\n%s [%s]\n%s - %s", fileName, line, caller, tag);
	
#else
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

#	if (ET_STOP_ON_OPENGL_ERROR)
		abort();
#	endif
	}
#endif
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
			
#if defined(GL_R8)
		CASE_VALUE(GL_R8)
#endif

#if defined(GL_RG8)
		CASE_VALUE(GL_RG8)
#endif
			
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
		
#if defined(GL_BGRA) && (GL_BGRA != GL_RGBA)
		CASE_VALUE(GL_BGRA)
#endif
			
#if defined(GL_RGB8) && (GL_RGB8 != GL_RGB)
		CASE_VALUE(GL_RGB8)
#endif
		
#if defined(GL_RGBA8) && (GL_RGBA8 != GL_RGBA)
		CASE_VALUE(GL_RGBA8)
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
#if !defined(ET_CONSOLE_APPLICATION)
	glDrawElementsInstanced(mode, count, type, indices, instanceCount);
	checkOpenGLError("glDrawElementsInstanced(%d, %d, %u, %08x, %d)", mode, count, type, indices, instanceCount);
	
#	if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += instanceCount * primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#	endif
#endif
}

#if defined(GL_ARB_draw_elements_base_vertex)
void et::etDrawElementsBaseVertex(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices, int base)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glDrawElementsBaseVertex(mode, count, type, indices, base);
	checkOpenGLError("glDrawElementsBaseVertex(mode, count, type, indices, base)");

#	if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#	endif
#endif
}
#else
void et::etDrawElementsBaseVertex(uint32_t, GLsizei, uint32_t, const GLvoid*, int)
{
	log::warning("Call to glDrawElementsBaseVertex without defined GL_ARB_draw_elements_base_vertex");
}
#endif

void et::etDrawElements(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glDrawElements(mode, count, type, indices);
	
	checkOpenGLError("glDrawElements(%s, %u, %s, 0x%08X)", glPrimitiveTypeToString(mode).c_str(),
		count, glTypeToString(type).c_str(), indices);

#	if ET_ENABLE_OPENGL_COUNTERS
	OpenGLCounters::primitiveCounter += primitiveCount(mode, static_cast<size_t>(count));
	++OpenGLCounters::DIPCounter;
#	endif
#endif
}

void et::etBindTexture(uint32_t target, uint32_t texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glBindTexture(target, texture);
	checkOpenGLError("glBindTexture(%s, %d)", glTexTargetToString(target).c_str(), texture);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindTextureCounter;
#	endif
#endif
}

void et::etBindBuffer(uint32_t target, uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glBindBuffer(target, buffer);
	checkOpenGLError("glBindBuffer(%u, %u)", target, buffer);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindBufferCounter;
#	endif
#endif
}

void et::etBindFramebuffer(uint32_t target, uint32_t framebuffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glBindFramebuffer(target, framebuffer);
	checkOpenGLError("glBindFramebuffer(%u, %u)", target, framebuffer);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindFramebufferCounter;
#	endif
#endif
}

void et::etViewport(int x, int y, GLsizei width, GLsizei height)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glViewport(x, y, width, height);
	checkOpenGLError("glViewport(%d, %d, %u, %u)", x, y, width, height);
#endif
}

void et::etUseProgram(uint32_t program)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glUseProgram(program);
	checkOpenGLError("glUseProgram(%u)", program);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::useProgramCounter;
#	endif
#endif
}

void et::etBindVertexArray(uint32_t arr)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glBindVertexArray(arr);
	checkOpenGLError("glBindVertexArray(%u)", arr);

#	if ET_ENABLE_OPENGL_COUNTERS
	++OpenGLCounters::bindVertexArrayObjectCounter;
#	endif
#endif
}

int32_t et::textureWrapValue(TextureWrap w)
{
	switch (w)
	{
		case TextureWrap::Repeat:
			return GL_REPEAT;
		case TextureWrap::ClampToEdge:
			return GL_CLAMP_TO_EDGE;
		case TextureWrap::MirrorRepeat:
			return GL_MIRRORED_REPEAT;
		default:
			ET_FAIL("Unrecognized texture wrap.");
	}
	
	return 0;
}

int32_t et::textureFiltrationValue(TextureFiltration f)
{
	switch (f)
	{
		case TextureFiltration::Nearest:
			return GL_NEAREST;
		case TextureFiltration::Linear:
			return GL_LINEAR;
		case TextureFiltration::NearestMipMapNearest:
			return GL_NEAREST_MIPMAP_NEAREST;
		case TextureFiltration::NearestMipMapLinear:
			return GL_NEAREST_MIPMAP_LINEAR;
		case TextureFiltration::LinearMipMapNearest:
			return GL_LINEAR_MIPMAP_NEAREST;
		case TextureFiltration::LinearMipMapLinear:
			return GL_LINEAR_MIPMAP_LINEAR;
		default:
			ET_FAIL("Unrecognized texture filtration.");
	}
	
	return 0;
}

uint32_t et::drawTypeValue(BufferDrawType t)
{
	switch (t)
	{
		case BufferDrawType::Static:
			return GL_STATIC_DRAW;
		case BufferDrawType::Dynamic:
			return GL_DYNAMIC_DRAW;
		case BufferDrawType::Stream:
			return GL_STREAM_DRAW;
		default:
			ET_FAIL("Unrecognized draw type");
	}
	
	return 0;
}

uint32_t et::primitiveTypeValue(PrimitiveType t)
{
	ET_ASSERT((t >= PrimitiveType::Points) && (t <= PrimitiveType::max))
	
	static const uint32_t conversion[static_cast<uint32_t>(PrimitiveType::max)] =
	{
		GL_POINTS,
		GL_LINES,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_LINE_STRIP
	};
	
	return conversion[static_cast<uint32_t>(t)];
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
#if !defined(ET_CONSOLE_APPLICATION)
	glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
	
	checkOpenGLError("glTexImage1D(%s, %d, %s, %d, %d, %s, %s, %, 0x%8X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(internalformat).c_str(),
		width, border, glInternalFormatToString(static_cast<int32_t>(format)).c_str(), glTypeToString(type).c_str(), pixels);
#endif
}

void et::etCompressedTexImage1D(uint32_t target, int level, uint32_t internalformat,
	GLsizei width, int border, GLsizei imageSize, const GLvoid * data)
{
#if !defined(ET_CONSOLE_APPLICATION)
	glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
	checkOpenGLError("glCompressedTexImage1D(%s, %d, %s, %d, %d, %d, 0x%8X)", glTexTargetToString(target).c_str(),
		level, glInternalFormatToString(static_cast<int32_t>(internalformat)).c_str(),width, border, imageSize, data);
#endif
}

#endif

void et::etCompressedTexImage2D(uint32_t target, int level, uint32_t internalformat,
	GLsizei width, GLsizei height, int border, GLsizei imageSize, const GLvoid * data)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(data);
	glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);

#	if (ET_DEBUG)
	checkOpenGLError("glCompressedTexImage2D(%s, %d, %s, %d, %d, %d, %d, 0x%016X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(static_cast<int32_t>(internalformat)).c_str(),
		width, height, border, imageSize, data);
#	endif
#endif
}

void et::etTexImage2D(uint32_t target, int level, int internalformat, GLsizei width, GLsizei height,
	int border, uint32_t format, uint32_t type, const GLvoid* pixels)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(pixels);
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);

#	if (ET_DEBUG)
	checkOpenGLError("glTexImage2D(%s, %d, %s, %d, %d, %d, %s, %s, %, 0x%08X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(internalformat).c_str(),
		width, height, border, glInternalFormatToString(format).c_str(), glTypeToString(type).c_str(),
		pixels);
#	endif
#endif
}

#if (ET_OPENGLES)

void et::etTexImage3D(uint32_t, int, int, GLsizei, GLsizei, GLsizei, int, uint32_t, uint32_t, const GLvoid*)
	{ ET_FAIL("Call to texImage3D in ES environment"); }

#else

void et::etTexImage3D(uint32_t target, int level, int internalformat, GLsizei width, GLsizei height,
	GLsizei depth, int border, uint32_t format, uint32_t type, const GLvoid* pixels)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(pixels);
	glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);

#	if (ET_DEBUG)
	checkOpenGLError("glTexImage3D(%s, %d, %s, %d, %d, %d, %d, %s, %s, %, 0x%08X)",
		glTexTargetToString(target).c_str(), level, glInternalFormatToString(internalformat).c_str(),
		width, height, depth, border, glInternalFormatToString(format).c_str(), glTypeToString(type).c_str(),
		pixels);
#	endif
#endif
}

#endif

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

const uint32_t* et::drawBufferTargets()
{
	static const uint32_t colorAttachmentValues[et::MaxDrawBuffers] =
	{
		GL_COLOR_ATTACHMENT0,
		
#	if defined(GL_COLOR_ATTACHMENT1)
		GL_COLOR_ATTACHMENT1,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT2)
		GL_COLOR_ATTACHMENT2,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT3)
		GL_COLOR_ATTACHMENT3,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT4)
		GL_COLOR_ATTACHMENT4,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT5)
		GL_COLOR_ATTACHMENT5,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT6)
		GL_COLOR_ATTACHMENT6,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
		
#	if defined(GL_COLOR_ATTACHMENT7)
		GL_COLOR_ATTACHMENT7,
#	else
		GL_COLOR_ATTACHMENT0,
#	endif
	};
	
	return colorAttachmentValues;
}

uint32_t et::drawBufferTarget(size_t i)
{
	ET_ASSERT(i < MaxDrawBuffers)
	return *(drawBufferTargets() + i);
}
