/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/opengl/opengl.h>
#include <et/core/flags.h>

namespace et
{
	enum OpenGLVersion
	{
		OpenGLVersion_unknown,
		OpenGLVersion_Old,
		OpenGLVersion_New,
		OpenGLVersion_max
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
	
	class OpenGLCapabilites : public Singleton<OpenGLCapabilites>, private FlagsHolder
	{ 
	public:
		bool hasFeature(OpenGLFeature value)
			{ return hasFlag(value); }
	
		OpenGLVersion version() const
			{ return _version; }

		const std::string& glslVersion() const
			{ return _glslVersion; }

		const std::string& glslVersionString() const
			{ return _glslVersionString; }

		size_t maxCubemapTextureSize() const
			{ return _maxCubemapTextureSize;}
		
		void checkCaps();

	private:
		std::string _openGlVersion;
		std::string _glslVersionString;
		std::string _glslVersion;

		size_t _maxCubemapTextureSize = 0;

		OpenGLVersion _version = OpenGLVersion_unknown;
	};
	
	inline OpenGLCapabilites& openGLCapabilites()
		{ return OpenGLCapabilites::instance(); }
}