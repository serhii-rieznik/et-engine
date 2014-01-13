/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et/opengl/openglcaps.h>

using namespace et;

bool OpenGLCapabilites::hasExtension(const std::string& e)
{
	return _extensions.count(lowercase(e)) > 0;
}

void OpenGLCapabilites::checkCaps()
{
	const char* glv = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	_versionShortString = std::string();
	_versionString = std::string(glv ? glv : "<Unknown OpenGL version>");
	if (glv != nullptr)
	{
		do
		{
			if ((_versionShortString.size() > 0) && (*glv == ET_SPACE)) break;
			
			if ((*glv >= '0') && (*glv <= '9'))
				_versionShortString.push_back(*glv);
		}
		while (*glv++);
	}
	while (_versionShortString.size() < 3)
		_versionShortString.push_back('0');
	checkOpenGLError("OpenGLCapabilites::checkCaps");
	
	const char* glslv = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	_glslVersionShortString = std::string();
	_glslVersionString = std::string(glslv ? glslv : "<Unknown GLSL version>");
	if (glslv != nullptr)
	{
		do
		{
			if ((_glslVersionShortString.size() > 0) && (*glslv == ET_SPACE)) break;

			if ((*glslv >= '0') && (*glslv <= '9'))
				_glslVersionShortString.push_back(*glslv);
		}
		while (*glslv++);
	}

	while (_glslVersionShortString.size() < 3)
		_glslVersionShortString.push_back('0');

	std::string lowercaseVersion = _versionString;
	lowercase(lowercaseVersion);
	
	_version = (strToInt(_glslVersionShortString) < 130) || (lowercaseVersion.find("es") != std::string::npos) ?
		OpenGLVersion_Old : OpenGLVersion_New;
	
	const char* ext = nullptr;
#if defined(GL_NUM_EXTENSIONS)
	if (_version == OpenGLVersion_Old)
#endif
	{
		ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
		checkOpenGLError("glGetString(GL_EXTENSIONS)");
		
		if (ext != nullptr)
		{
			std::string extString(ext);
			
			size_t spacePosition = 0;
			do
			{
				spacePosition = extString.find(ET_SPACE);
				if (spacePosition == std::string::npos)
					break;
				
				std::string extension = lowercase(extString.substr(0, spacePosition));
				_extensions[extension] = 1;
				extString.erase(0, spacePosition + 1);
			}
			while ((extString.size() > 0) && (spacePosition != std::string::npos));
			
			trim(extString);
			
			if (extString.size() > 0)
			{
				lowercase(extString);
				_extensions[extString] = 1;
			}
		}
	}
#if defined(GL_NUM_EXTENSIONS)
	else
	{
		int numExtensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		checkOpenGLError("glGetIntegerv(GL_NUM_EXTENSIONS, ...");
		for (int i = 0; i < numExtensions; ++i)
		{
			ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
			_extensions[lowercase(ext)] = 1;
		}
	}
#endif
	
	int maxSize = 0;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxSize);
	checkOpenGLError("glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, ...");
	_maxCubemapTextureSize = static_cast<uint32_t>(maxSize);

	maxSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	checkOpenGLError("glGetIntegerv(GL_MAX_TEXTURE_SIZE, ...");
	_maxTextureSize = static_cast<uint32_t>(maxSize);
	
	int maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	checkOpenGLError("glGetIntegerv(GL_MAX_SAMPLES, ...");
	_maxSamples = static_cast<uint32_t>(maxSamples);
	
	int maxUnits = 0;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxUnits);
	checkOpenGLError("glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, ...");
	if (maxUnits > 0)
		setFlag(OpenGLFeature_VertexTextureFetch);

#if defined(GL_ARB_draw_elements_base_vertex)
	setFlag(OpenGLFeature_DrawElementsBaseVertex);
#endif
	
	if (glGenerateMipmap != nullptr)
		setFlag(OpenGLFeature_MipMapGeneration);
	
#if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	bool stillSupport = (glGenVertexArrays != nullptr) && (glDeleteVertexArrays != nullptr)
		&& (glBindVertexArray != nullptr) && (glIsVertexArray != nullptr);
	
	if (stillSupport)
	{
		uint32_t testArray = 0;
		glGenVertexArrays(1, &testArray);
		if ((glGetError() == GL_NO_ERROR) && (testArray != 0))
		{
			glDeleteVertexArrays(1, &testArray);
			setFlag(OpenGLFeature_VertexArrayObjects);
		}
	}
#endif
	
	log::info("[OpenGLCapabilites] Version: %s (%s), GLSL version: %s (%s)", _versionString.c_str(),
		_versionShortString.c_str(), _glslVersionString.c_str(), _glslVersionShortString.c_str());
};
