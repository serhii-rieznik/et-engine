/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/flags.h>
#include <et/rendering/rendering.h>
#include <et/rendering/opengl/opengl.h>

namespace et
{
	class OpenGLCapabilities : public Singleton<OpenGLCapabilities>, private FlagsHolder
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
		
		bool hasExtension(const std::string&);
		
		void checkCaps();
		
	private:
		ET_SINGLETON_CONSTRUCTORS(OpenGLCapabilities)
		
	private:
		std::string _versionString;
		std::string _versionShortString;
		std::string _glslVersionString;
		std::string _glslVersionShortString;
		
		std::map<std::string, int> _extensions;
		
		OpenGLVersion _version = OpenGLVersion::Unknown;
		bool _isOpenGLES = false;
	};
}
