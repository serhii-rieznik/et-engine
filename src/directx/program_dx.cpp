/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/program.h>

using namespace et;

static const std::string etNoShader = "none";

Program::Program(RenderContext* rc) : _rc(rc),
	_mModelViewLocation(-1), _mModelViewProjectionLocation(-1), _vCameraLocation(-1),
	_vPrimaryLightLocation(-1), _mLightProjectionMatrixLocation(-1), _mTransformLocation(-1)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create Program in console application");
#endif
}

Program::Program(RenderContext* rc, const std::string& vertexShader, const std::string& geometryShader,
	const std::string& fragmentShader, const std::string& objName, const std::string& origin,
	const StringList& defines) : APIObject(objName, origin), _rc(rc), _mModelViewLocation(-1),
	_mModelViewProjectionLocation(-1), _vCameraLocation(-1), _vPrimaryLightLocation(-1),
	_mLightProjectionMatrixLocation(-1), _mTransformLocation(-1), _defines(defines)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create Program in console application");
#else
	buildProgram(vertexShader, geometryShader, fragmentShader);
#endif
}

Program::~Program()
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t program = static_cast<uint32_t>(apiHandle());
	if (program != 0)
	{
		_rc->renderState().programDeleted(program);
	}
#endif
}

Program::UniformMap::const_iterator Program::findUniform(const std::string& name) const
{
	return _uniforms.end();
}

int Program::getUniformLocation(const std::string& uniform) const
{
	return -1;
}

uint32_t Program::getUniformType(const std::string& uniform) const
{
	return 0;
}

Program::Uniform Program::getUniform(const std::string& uniform) const
{
	return Program::Uniform();
}

void Program::setModelViewMatrix(const mat4& m, bool forced)
{
}

void Program::setMVPMatrix(const mat4& m, bool forced)
{
}

void Program::setCameraPosition(const vec3& p, bool forced)
{
}

void Program::setPrimaryLightPosition(const vec3& p, bool forced)
{
}

void Program::setLightProjectionMatrix(const mat4& m, bool forced)
{
}

void Program::setTransformMatrix(const mat4 &m, bool forced)
{
}

void Program::setCameraProperties(const Camera& cam)
{
	setModelViewMatrix(cam.modelViewMatrix());
	setMVPMatrix(cam.modelViewProjectionMatrix());
	setCameraPosition(cam.position());
}

void Program::buildProgram(const std::string& vertex_source, const std::string& geom_source,
	const std::string& frag_source)
{
	_floatCache.clear();
	_vec2Cache.clear();
	_vec3Cache.clear();
	_vec4Cache.clear();
	_mat3Cache.clear();
	_mat4Cache.clear();
	_uniforms.clear();
	
#if !defined(ET_CONSOLE_APPLICATION)

#endif
}

int Program::link()
{
	int result = 0;

#if !defined(ET_CONSOLE_APPLICATION)

#endif
	
	return result;
}

void Program::validate() const
{
}

/*
 * Uniform setters
 */

void Program::setUniform(int nLoc, uint32_t type, int32_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, uint32_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, int64_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, uint64_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const unsigned long value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const float value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec2& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec3& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec4& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const vec4& value)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat3& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat4& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const mat4& value)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec2* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec3* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec4* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat4* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
