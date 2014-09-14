/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
		OpenGLVersion_2x,
		OpenGLVersion_3x,
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
		
		bool isOpenGLES() const
			{ return _isOpenGLES; }
	
		OpenGLVersion version() const
			{ return _version; }

		const std::string& versionString() const
			{ return _versionString; }

		const std::string& versionShortString() const
			{ return _versionShortString; }
		
		const std::string& glslVersion() const
			{ return _glslVersionString; }
		
		const std::string& glslVersionShortString() const
			{ return _glslVersionShortString; }

		uint32_t maxTextureSize() const
			{ return _maxTextureSize;}
		
		uint32_t maxCubemapTextureSize() const
			{ return _maxCubemapTextureSize;}
		
		uint32_t maxSamples() const
			{ return _maxSamples; }
		
		bool hasExtension(const std::string&);
		
		void checkCaps();

	private:
		std::string _versionString;
		std::string _versionShortString;
		std::string _glslVersionString;
		std::string _glslVersionShortString;
		
		std::map<std::string, int> _extensions;

		uint32_t _maxTextureSize = 0;
		uint32_t _maxCubemapTextureSize = 0;
		uint32_t _maxSamples = 0;

		OpenGLVersion _version = OpenGLVersion_unknown;
		bool _isOpenGLES = false;
	};
	
	inline OpenGLCapabilites& openGLCapabilites()
		{ return OpenGLCapabilites::instance(); }
}
