//
//  openglfix.h
//  Jewellery Lab
//
//  Created by Sergey Reznik on 3/1/2015.
//  Copyright (c) 2015 Sergey Reznik. All rights reserved.
//

#pragma once

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define ET_OPENGLES									1

#if !defined(GL_ES_VERSION_3_0)
#	define glGenVertexArrays						glGenVertexArraysOES
#	define glBindVertexArray						glBindVertexArrayOES
#	define glIsVertexArray							glIsVertexArrayOES
#	define glDeleteVertexArrays						glDeleteVertexArraysOES
#	define glRenderbufferStorageMultisample			glRenderbufferStorageMultisampleAPPLE
#	define glDrawElementsInstanced					glDrawElementsInstancedEXT
#endif

#define glClearDepth								glClearDepthf
#define glDepthRange								glDepthRangef
#define glMapBufferRange							glMapBufferRangeEXT
#define glFlushMappedBufferRange					glFlushMappedBufferRangeEXT
#define glMapBuffer									glMapBufferOES
#define glUnmapBuffer								glUnmapBufferOES

#if !defined(GL_DEPTH_COMPONENT24) && defined(GL_DEPTH_COMPONENT24_OES)
#	define GL_DEPTH_COMPONENT24						GL_DEPTH_COMPONENT24_OES
#endif

#if !defined(GL_DEPTH_COMPONENT32) && defined(GL_DEPTH_COMPONENT32_OES)
#	define GL_DEPTH_COMPONENT32						GL_DEPTH_COMPONENT32_OES
#endif

#if !defined(GL_RED) && defined(GL_RED_EXT)
#	define GL_RED									GL_RED_EXT
#endif

#if !defined(GL_R) && defined(GL_R_EXT)
#	define GL_R										GL_R_EXT
#endif

#if !defined(GL_R8) && defined(GL_R8_EXT)
#	define GL_R8									GL_R8_EXT
#endif

#if !defined(GL_R16) && defined(GL_R16_EXT)
#	define GL_R16									GL_R16_EXT
#endif

#if !defined(GL_RG) && defined(GL_RG_EXT)
#	define GL_RG									GL_RG_EXT
#endif

#if !defined(GL_RG8) && defined(GL_RG8_EXT)
#	define GL_RG8									GL_RG8_EXT
#endif

#if !defined(GL_RG16) && defined(GL_RG16_EXT)
#	define GL_RG16									GL_RG16_EXT
#endif

#if !defined(GL_HALF_FLOAT)
#	define GL_HALF_FLOAT							GL_HALF_FLOAT_OES
#endif

#if !defined(GL_RGB8)
#	define GL_RGB8									GL_RGB8_OES
#endif

#if !defined(GL_RGBA8)
#	define GL_RGBA8									GL_RGBA8_OES
#endif

#if !defined(GL_RGBA32F) && defined(GL_RGBA32F_EXT)
#	define GL_RGBA32F								GL_RGBA32F_EXT
#endif

#if !defined(GL_RGB32F) && defined(GL_RGB32F_EXT)
#	define GL_RGB32F								GL_RGB32F_EXT
#endif

#if !defined(GL_RG32F) && defined(GL_RG32F_EXT)
#	define GL_RG32F									GL_RG32F_EXT
#endif

#if !defined(GL_R32F) && defined(GL_R32F_EXT)
#	define GL_R32F									GL_R32F_EXT
#endif

#if !defined(GL_RGBA162F) && defined(GL_RGBA16F_EXT)
#	define GL_RGBA16F								GL_RGBA16F_EXT
#endif

#if !defined(GL_RGB16F) && defined(GL_RGB16F_EXT)
#	define GL_RGB16F								GL_RGB16F_EXT
#endif

#if !defined(GL_RG16F) && defined(GL_RG16F_EXT)
#	define GL_RG16F									GL_RG16F_EXT
#endif

#if !defined(GL_R16F) && defined(GL_R16F_EXT)
#	define GL_R16F									GL_R16F_EXT
#endif

#if !defined(GL_TEXTURE_MAX_LEVEL)
#	define GL_TEXTURE_MAX_LEVEL						GL_TEXTURE_MAX_LEVEL_APPLE
#endif

#if !defined(GL_VERTEX_ARRAY_BINDING)
#	define GL_VERTEX_ARRAY_BINDING					GL_VERTEX_ARRAY_BINDING_OES
#endif

#if !defined(GL_MAX_SAMPLES)
#	define GL_MAX_SAMPLES							GL_MAX_SAMPLES_APPLE
#endif

#if !defined(GL_READ_FRAMEBUFFER)
#	define GL_READ_FRAMEBUFFER						GL_READ_FRAMEBUFFER_APPLE
#endif

#if !defined(GL_READ_FRAMEBUFFER_BINDING)
#	define GL_READ_FRAMEBUFFER_BINDING				GL_READ_FRAMEBUFFER_BINDING_APPLE
#endif

#if !defined(GL_DRAW_FRAMEBUFFER)
#	define GL_DRAW_FRAMEBUFFER						GL_DRAW_FRAMEBUFFER_APPLE
#endif

#if !defined(GL_DRAW_FRAMEBUFFER_BINDING)
#	define GL_DRAW_FRAMEBUFFER_BINDING				GL_DRAW_FRAMEBUFFER_BINDING_APPLE
#endif

#if !defined(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
#	define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE	GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE
#endif

#if !defined(GL_MAP_WRITE_BIT) && defined(GL_MAP_WRITE_BIT_EXT)
#	define GL_MAP_WRITE_BIT							GL_MAP_WRITE_BIT_EXT
#endif

#if !defined(GL_MAP_READ_BIT) && defined(GL_MAP_READ_BIT_EXT)
#	define GL_MAP_READ_BIT							GL_MAP_READ_BIT_EXT
#endif

#if !defined(GL_WRITE_ONLY) && defined(GL_WRITE_ONLY_OES)
#	define GL_WRITE_ONLY							GL_WRITE_ONLY_OES
#endif

#if !defined(GL_MAP_INVALIDATE_BUFFER_BIT) && defined(GL_MAP_INVALIDATE_BUFFER_BIT_EXT)
#	define GL_MAP_INVALIDATE_BUFFER_BIT				GL_MAP_INVALIDATE_BUFFER_BIT_EXT
#endif

#if !defined(GL_MAP_FLUSH_EXPLICIT_BIT) && defined(GL_MAP_FLUSH_EXPLICIT_BIT_EXT)
#	define GL_MAP_FLUSH_EXPLICIT_BIT				GL_MAP_FLUSH_EXPLICIT_BIT_EXT
#endif

#if !defined(GL_MAP_UNSYNCHRONIZED_BIT) && defined(GL_MAP_UNSYNCHRONIZED_BIT_EXT)
#	define GL_MAP_UNSYNCHRONIZED_BIT				GL_MAP_UNSYNCHRONIZED_BIT_EXT
#endif
