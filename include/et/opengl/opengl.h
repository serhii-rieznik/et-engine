/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

#if ET_PLATFORM_WIN
#
#	include <et\platform-win\glee.h>
#
#	define ET_OPENGLES									0
#
#elif ET_PLATFORM_MAC
#
#	include <OpenGL/gl3.h>
#	include <OpenGL/gl3ext.h>
#
#	define ET_OPENGLES									0
#
#elif ET_PLATFORM_IOS
#
#	include <et/platform-ios/opengl.ios.h>
#
#elif (ET_PLATFORM_ANDROID)
#
#	include <EGL/egl.h>
#	include <GLES3/gl3.h>
#	include <GLES3/gl3ext.h>
#	include <GLES3/gl3platform.h>
#
#	define ET_OPENGLES								1
#
#	define glClearDepth								glClearDepthf
#	define glDepthRange								glDepthRangef
#
#	if !defined(GL_BGRA)
#		if defined(GL_BGRA_EXT)
#			define GL_BGRA							GL_BGRA_EXT
#		else
#			define GL_BGRA							GL_RGBA
#		endif
#	endif
#
#else
#
#	error Platform is not defined
#
#endif

#if !defined(GL_COLOR_ATTACHMENT0)
#
#	define GL_COLOR_ATTACHMENT0						0
#
#endif

#if !defined(GL_COMPARE_REF_TO_TEXTURE) && defined(GL_COMPARE_R_TO_TEXTURE)
#
#	define GL_COMPARE_REF_TO_TEXTURE				GL_COMPARE_R_TO_TEXTURE
#
#endif

#if !defined(GL_READ_ONLY)
#	define GL_READ_ONLY 0
#endif

#if !defined(GL_READ_WRITE)
#	define GL_READ_WRITE 0
#endif

#if (ET_DEBUG)
#
#	define checkOpenGLError(...)				et::checkOpenGLErrorEx(ET_CALL_FUNCTION, __FILE__, \
													ET_TO_CONST_CHAR(__LINE__), __VA_ARGS__);
#
#	define ET_OPENGL_DEBUG_SCOPE_IN_DEBUG		OpenGLDebugScope etOpenGLDebugScope(ET_CALL_FUNCTION);
#
#else
#
#	define checkOpenGLError(...)
#	define ET_OPENGL_DEBUG_SCOPE_IN_DEBUG		
#
#endif

#define ET_ENABLE_OPENGL_COUNTERS				1

#if !(defined(GL_ES_VERSION_3_0) || defined(GL_ARB_vertex_array_object) || defined(GL_OES_vertex_array_object))
#	error Vertex Array Objects are not supported on selected platform
#endif

#define ET_OPENGL_DEBUG_SCOPE					OpenGLDebugScope etOpenGLDebugScope(ET_CALL_FUNCTION);


namespace et
{
	struct OpenGLCounters
	{
		static size_t primitiveCounter;
		static size_t DIPCounter;
		static size_t bindTextureCounter;
		static size_t bindBufferCounter;
		static size_t bindFramebufferCounter;
		static size_t useProgramCounter;
		static size_t bindVertexArrayObjectCounter;
		static void reset();
	};
	
	enum class OpenGLVersion
	{
		Unknown,
		Version_2x,
		Version_3x,
		max
	};
	
	enum OpenGLFeature
	{
		OpenGLFeature_MipMapGeneration = 0x00000001,
		OpenGLFeature_VertexAttribArrays = 0x00000002,
		OpenGLFeature_VertexBufferObjects = 0x00000004,
		OpenGLFeature_DrawElementsBaseVertex = 0x00000008,
		OpenGLFeature_VertexArrayObjects = 0x00000010,
		OpenGLFeature_VertexTextureFetch = 0x00000020,
	};
	
	struct OpenGLDebugScope
	{
		OpenGLDebugScope(const std::string&);
		~OpenGLDebugScope();
	};

	void checkOpenGLErrorEx(const char* caller, const char* fileName, const char* line, const char* tag, ...);

	std::string glErrorToString(uint32_t error);
	std::string glTexTargetToString(uint32_t target);
	std::string glInternalFormatToString(int32_t format);
	std::string glTypeToString(uint32_t type);
	std::string glBlendFuncToString(uint32_t value);
	std::string glPrimitiveTypeToString(uint32_t value);
	
	void validateExtensions();

	VertexAttributeType openglTypeToVertexAttributeType(uint32_t);
	uint32_t vertexAttributeTypeValue(VertexAttributeType);
	
	uint32_t textureTargetValue(TextureTarget);
	uint32_t textureFormatValue(TextureFormat);
	uint32_t dataTypeValue(DataType);
	uint32_t primitiveTypeValue(PrimitiveType);
	
	size_t primitiveCount(uint32_t mode, size_t count);

	void etViewport(int x, int y, GLsizei width, GLsizei height);
	void etDrawElements(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices);
	void etDrawElementsInstanced(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices, GLsizei instanceCount);
	void etDrawElementsBaseVertex(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices, int base);
	void etBindTexture(uint32_t target, uint32_t texture);
	void etBindBuffer(uint32_t target, uint32_t buffer);
	void etBindFramebuffer(uint32_t target, uint32_t framebuffer);
	void etUseProgram(uint32_t program);
	void etBindVertexArray(uint32_t arr);

	void etTexImage1D(uint32_t target, int level, int internalformat, GLsizei width, int border,
		uint32_t format, uint32_t type, const GLvoid * pixels);
	
	void etTexImage2D(uint32_t target, int level, int internalformat, GLsizei width, GLsizei height,
		int border, uint32_t format, uint32_t type, const GLvoid * pixels);
	
	void etCompressedTexImage1D(uint32_t target, int level, uint32_t internalformat, GLsizei width,
		int border, GLsizei imageSize, const GLvoid * data);
	
	void etCompressedTexImage2D(uint32_t target, int level, uint32_t internalformat, GLsizei width,
		GLsizei height, int border, GLsizei imageSize, const GLvoid * data);

	int32_t textureWrapValue(TextureWrap);
	int32_t textureFiltrationValue(TextureFiltration);
	
	uint32_t drawTypeValue(BufferDrawType);
	uint32_t primitiveTypeValue(PrimitiveType);
		
	const uint32_t* drawBufferTargets();
	uint32_t drawBufferTarget(size_t);
}
