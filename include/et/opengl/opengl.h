/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

#if ET_PLATFORM_WIN
#
#	include <et\platform-win\glee.h>
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
#	include <OpenGLES/ES2/gl.h>
#	include <OpenGLES/ES2/glext.h>
#
#	define ET_OPENGLES									1
#
#	if !defined(GL_ES_VERSION_3_0)
#		define glGenVertexArrays						glGenVertexArraysOES
#		define glBindVertexArray						glBindVertexArrayOES
#		define glIsVertexArray							glIsVertexArrayOES
#		define glDeleteVertexArrays						glDeleteVertexArraysOES
#		define glRenderbufferStorageMultisample			glRenderbufferStorageMultisampleAPPLE
#		define glDrawElementsInstanced					glDrawElementsInstancedEXT
#	endif
#
#	define glClearDepth									glClearDepthf
#	define glDepthRange									glDepthRangef
#	define glMapBufferRange								glMapBufferRangeEXT
#	define glFlushMappedBufferRange						glFlushMappedBufferRangeEXT
#	define glMapBuffer									glMapBufferOES
#	define glUnmapBuffer								glUnmapBufferOES
#
#	if !defined(GL_DEPTH_COMPONENT24)
#		define GL_DEPTH_COMPONENT24						GL_DEPTH_COMPONENT24_OES
#	endif
#
#	if !defined(GL_RED) && defined(GL_RED_EXT)
#		define GL_RED									GL_RED_EXT
#	endif
#
#	if !defined(GL_RG) && defined(GL_RG_EXT)
#		define GL_RG									GL_RG_EXT
#	endif
#
#	if !defined(GL_HALF_FLOAT)
#		define GL_HALF_FLOAT							GL_HALF_FLOAT_OES
#	endif
#
#	if !defined(GL_RGB8)
#		define GL_RGB8									GL_RGB8_OES
#	endif
#
#	if !defined(GL_RGBA8)
#		define GL_RGBA8									GL_RGBA8_OES
#	endif
#
#	if !defined(GL_RGBA32F) && defined(GL_RGBA32F_EXT)
#		define GL_RGBA32F								GL_RGBA32F_EXT
#	endif
#
#	if !defined(GL_RGB32F) && defined(GL_RGB32F_EXT)
#		define GL_RGB32F								GL_RGB32F_EXT
#	endif
#
#	if !defined(GL_RG32F) && defined(GL_RG32F_EXT)
#		define GL_RG32F									GL_RG32F_EXT
#	endif
#
#	if !defined(GL_R32F) && defined(GL_R32F_EXT)
#		define GL_R32F									GL_R32F_EXT
#	endif
#
#	if !defined(GL_RGBA162F) && defined(GL_RGBA16F_EXT)
#		define GL_RGBA16F								GL_RGBA16F_EXT
#	endif
#
#	if !defined(GL_RGB16F) && defined(GL_RGB16F_EXT)
#		define GL_RGB16F								GL_RGB16F_EXT
#	endif
#
#	if !defined(GL_RG16F) && defined(GL_RG16F_EXT)
#		define GL_RG16F									GL_RG16F_EXT
#	endif
#
#	if !defined(GL_R16F) && defined(GL_R16F_EXT)
#		define GL_R16F									GL_R16F_EXT
#	endif
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
#	if !defined(GL_MAP_WRITE_BIT) && defined(GL_MAP_WRITE_BIT_EXT)
#		define GL_MAP_WRITE_BIT GL_MAP_WRITE_BIT_EXT
#	endif
#
#	if !defined(GL_MAP_READ_BIT) && defined(GL_MAP_READ_BIT_EXT)
#		define GL_MAP_READ_BIT GL_MAP_READ_BIT_EXT
#	endif
#
#	if !defined(GL_WRITE_ONLY) && defined(GL_WRITE_ONLY_OES)
#		define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#	endif
#
#	if !defined(GL_MAP_INVALIDATE_BUFFER_BIT) && defined(GL_MAP_INVALIDATE_BUFFER_BIT_EXT)
#		define GL_MAP_INVALIDATE_BUFFER_BIT GL_MAP_INVALIDATE_BUFFER_BIT_EXT
#	endif
#
#	if !defined(GL_MAP_FLUSH_EXPLICIT_BIT) && defined(GL_MAP_FLUSH_EXPLICIT_BIT_EXT)
#		define GL_MAP_FLUSH_EXPLICIT_BIT GL_MAP_FLUSH_EXPLICIT_BIT_EXT
#	endif
#
#	if !defined(GL_MAP_UNSYNCHRONIZED_BIT) && defined(GL_MAP_UNSYNCHRONIZED_BIT_EXT)
#		define GL_MAP_UNSYNCHRONIZED_BIT GL_MAP_UNSYNCHRONIZED_BIT_EXT
#	endif
#
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
#			define GL_BGRA								GL_BGRA_EXT
#		else
#			define GL_BGRA								GL_RGBA
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
#
#	define checkOpenGLError(...)		et::checkOpenGLErrorEx(ET_CALL_FUNCTION, __FILE__, \
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

#define ET_ENABLE_OPENGL_COUNTERS	1

#if defined(GL_ES_VERSION_3_0) || defined(GL_ARB_vertex_array_object) || defined(GL_OES_vertex_array_object)
#	define ET_SUPPORT_VERTEX_ARRAY_OBJECTS	1
#else
#	error Vertex Array Objects are not supported on selected platform
#endif

#define ET_OPENGL_DEBUG_SCOPE		OpenGLDebugScope etOpenGLDebugScope(ET_CALL_FUNCTION);

#define ET_OPENGL_INCLUDES
#	include <et/opengl/opengltypes.h>
#undef ET_OPENGL_INCLUDES

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
	
	size_t bitsPerPixelForType(uint32_t type);
	size_t bitsPerPixelForTextureFormat(uint32_t internalFormat, uint32_t type);
	size_t channelsForTextureFormat(uint32_t internalFormat);
	
	const uint32_t* drawBufferTargets();
	uint32_t drawBufferTarget(size_t);
}
