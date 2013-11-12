/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/opengl/opengltypes.h>

#if ET_PLATFORM_WIN
#
#	include <et/platform-win/opengl.win.h>
#
#	define ET_OPENGLES								0
#
#elif ET_PLATFORM_MAC
#
#	include <OpenGL/OpenGL.h>
#	if defined(CGL_VERSION_1_3)
#
#		include <OpenGL/gl3.h>
#		include <OpenGL/gl3ext.h>
#
#	elif defined(CGL_VERSION_1_2)
#
#		define glBindFragDataLocation				glBindFragDataLocationEXT
#		define glGenVertexArrays					glGenVertexArraysAPPLE
#		define glBindVertexArray					glBindVertexArrayAPPLE
#		define glIsVertexArray						glIsVertexArrayAPPLE
#		define glDeleteVertexArrays					glDeleteVertexArraysAPPLE
#		define GL_VERTEX_ARRAY_BINDING				GL_VERTEX_ARRAY_BINDING_APPLE
#
#	else
#
#		error Unsupported OpenGL version
#
#	endif
#
#	define ET_OPENGLES								0
#
#elif ET_PLATFORM_IOS
#
#	if (0 && defined(__IPHONE_7_0))
#		include <OpenGLES/ES3/gl.h>
#		include <OpenGLES/ES3/glext.h>
#	else
#		include <OpenGLES/ES2/gl.h>
#		include <OpenGLES/ES2/glext.h>
#	endif
#
#	define ET_OPENGLES									1
#
#	if !defined(GL_DEPTH_COMPONENT24)
#		define GL_DEPTH_COMPONENT24						GL_DEPTH_COMPONENT24_OES
#	endif
#
#	if !defined(GL_HALF_FLOAT)
#		define GL_HALF_FLOAT							GL_HALF_FLOAT_OES
#	endif
#
#	if !defined(GL_RGBA8)
#		define GL_RGBA8									GL_RGBA8_OES
#	endif
#
#	if !defined(GL_RGB32F)
#		define GL_RGB32F								GL_RGB32F_EXT
#	endif
#
#	if !defined(GL_RGBA32F)
#		define GL_RGBA32F								GL_RGBA32F_EXT
#	endif
#
#	if !defined(GL_ES_VERSION_3_0)
#		define glGenVertexArrays						glGenVertexArraysOES
#		define glBindVertexArray						glBindVertexArrayOES
#		define glIsVertexArray							glIsVertexArrayOES
#		define glDeleteVertexArrays						glDeleteVertexArraysOES
#		define glRenderbufferStorageMultisample			glRenderbufferStorageMultisampleAPPLE
#	endif
#
#	define glClearDepth									glClearDepthf
#
#	if !defined(GL_TEXTURE_MAX_LEVEL)
#		define GL_TEXTURE_MAX_LEVEL						GL_TEXTURE_MAX_LEVEL_APPLE
#	endif
#
#	if !defined(GL_VERTEX_ARRAY_BINDING)
#		define GL_VERTEX_ARRAY_BINDING					GL_VERTEX_ARRAY_BINDING_OES
#	endif
#
#	if !defined(GL_MAX_SAMPLES)
#		define GL_MAX_SAMPLES							GL_MAX_SAMPLES_APPLE
#	endif
#
#	if !defined(GL_READ_FRAMEBUFFER)
#		define GL_READ_FRAMEBUFFER						GL_READ_FRAMEBUFFER_APPLE
#	endif
#
#	if !defined(GL_READ_FRAMEBUFFER_BINDING)
#		define GL_READ_FRAMEBUFFER_BINDING				GL_READ_FRAMEBUFFER_BINDING_APPLE
#	endif
#
#	if !defined(GL_DRAW_FRAMEBUFFER)
#		define GL_DRAW_FRAMEBUFFER						GL_DRAW_FRAMEBUFFER_APPLE
#	endif
#
#	if !defined(GL_DRAW_FRAMEBUFFER_BINDING)
#		define GL_DRAW_FRAMEBUFFER_BINDING				GL_DRAW_FRAMEBUFFER_BINDING_APPLE
#	endif
#
#	if !defined(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
#		define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE	GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE
#	endif
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
#
#else
#
#	error Platform is not defined
#
#endif

#if !defined(GL_VERSION_3_2)
#
#	define glFramebufferTexture(target, attachment, texture, level) \
			glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level)
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

#if (ET_DEBUG)
#
#	define ET_ENABLE_OPENGL_COUNTERS	1
#	define checkOpenGLError(...) \
		checkOpenGLErrorEx(ET_CALL_FUNCTION, __FILE__, ET_TO_CONST_CHAR(__LINE__), __VA_ARGS__);
#
#else
#
#	define ET_ENABLE_OPENGL_COUNTERS	0
#	define checkOpenGLError(...)
#
#endif

#if defined(GL_ES_VERSION_3_0) || defined(GL_ARB_vertex_array_object) || defined(GL_OES_vertex_array_object)
#	define ET_SUPPORT_VERTEX_ARRAY_OBJECTS	1
#else
#	error Vertex Array Objects are not supported on selected platform
#endif

#define ET_OPENGL_DEBUG_FUNCTION_SCOPE		OpenGLDebugScope etOpenGLDebugScope(ET_CALL_FUNCTION);

namespace et
{
	enum VertexAttributeType
	{
		Type_Undefined = -1,

		Type_Float = GL_FLOAT,
		Type_Vec2 = GL_FLOAT_VEC2,
		Type_Vec3 = GL_FLOAT_VEC3,
		Type_Vec4 = GL_FLOAT_VEC4,
		Type_Mat3 = GL_FLOAT_MAT3,
		Type_Mat4 = GL_FLOAT_MAT4,
		Type_Int = GL_INT
	};

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

	uint32_t primitiveTypeValue(PrimitiveType);
	size_t primitiveCount(uint32_t mode, size_t count);

	void etViewport(int x, int y, GLsizei width, GLsizei height);
	void etDrawElements(uint32_t mode, GLsizei count, uint32_t type, const GLvoid* indices);
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
	
	size_t bitsPerPixelForType(uint32_t type);
	size_t bitsPerPixelForTextureFormat(uint32_t internalFormat, uint32_t type);
}
