/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et/opengl/openglcaps.h>

using namespace et;

void OpenGLCapabilites::checkCaps()
{
	const char* glv = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	const char* glslv = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	checkOpenGLError("OpenGLCapabilites::checkCaps");
	
	_glslVersion = std::string();
	_openGlVersion = std::string(glv ? glv : "<Unknown OpenGL version>");
	_glslVersionString = std::string(glslv ? glslv : "<Unknown GLSL version>");

	if (glslv != nullptr)
	{
		do
		{
			if ((_glslVersion.size() > 0) && (*glslv == ET_SPACE)) break;

			if ((*glslv >= '0') && (*glslv <= '9'))
				_glslVersion.push_back(*glslv);
		}
		while (*glslv++);
	}

	if (_glslVersion.size() < 3)
		_glslVersion.push_back('0');
	
	_version = strToInt(_glslVersion) < 130 ? OpenGLVersion_Old : OpenGLVersion_New;

	int maxSize = 0;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxSize);
	_maxCubemapTextureSize = static_cast<size_t>(maxSize);
	
	int maxUnits = 0;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxUnits);
	
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
	
	log::info("[OpenGLCapabilites] Version: %s, GLSL version: %s (%s)", _openGlVersion.c_str(),
		_glslVersionString.c_str(), _glslVersion.c_str());
};
