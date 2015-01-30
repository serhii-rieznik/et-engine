/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/app/application.h>
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
		if (glIsProgram(program))
		{
			glDeleteProgram(program);
			checkOpenGLError("glDeleteProgram: %s", name().c_str());
		}
	}
#endif
}

Program::UniformMap::const_iterator Program::findUniform(const std::string& name) const
{
	ET_ASSERT(apiHandleValid());
	return _uniforms.find(name);
}

int Program::getUniformLocation(const std::string& uniform) const
{
	ET_ASSERT(apiHandleValid());

	auto i = findUniform(uniform);
	return (i == _uniforms.end()) ? -1 : i->second.location;
}

uint32_t Program::getUniformType(const std::string& uniform) const
{
	ET_ASSERT(apiHandleValid());

	auto i = findUniform(uniform);
	return (i == _uniforms.end()) ? 0 : i->second.type;
}

Program::Uniform Program::getUniform(const std::string& uniform) const
{
	ET_ASSERT(apiHandleValid());

	auto i = findUniform(uniform);
	return (i == _uniforms.end()) ? Program::Uniform() : i->second;
}

void Program::setModelViewMatrix(const mat4& m, bool forced)
{
	setUniform(_mModelViewLocation, GL_FLOAT_MAT4, m, forced);
}

void Program::setMVPMatrix(const mat4& m, bool forced)
{
	setUniform(_mModelViewProjectionLocation, GL_FLOAT_MAT4, m, forced);
}

void Program::setCameraPosition(const vec3& p, bool forced)
{
	setUniform(_vCameraLocation, GL_FLOAT_VEC3, p, forced);
}

void Program::setPrimaryLightPosition(const vec3& p, bool forced)
{
	setUniform(_vPrimaryLightLocation, GL_FLOAT_VEC3, p, forced);
}

void Program::setLightProjectionMatrix(const mat4& m, bool forced)
{
	setUniform(_mLightProjectionMatrixLocation, GL_FLOAT_MAT4, m, forced);
}

void Program::setTransformMatrix(const mat4 &m, bool forced)
{
	setUniform(_mTransformLocation, GL_FLOAT_MAT4, m, forced);
}

void Program::setCameraProperties(const Camera& cam)
{
	ET_ASSERT(apiHandleValid());

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

#	if (ET_OPENGLES)
	if (geom_source.length() && (geom_source != etNoShader))
		log::info("[Program] Geometry shader skipped in OpenGL ES");
#	endif
	
	checkOpenGLError("[Program] buildProgram - %s", name().c_str());

	uint32_t program = static_cast<uint32_t>(apiHandle());
	
	if ((program == 0) || !glIsProgram(program))
	{
		program = glCreateProgram();
		checkOpenGLError("glCreateProgram - %s", name().c_str());
		
		setAPIHandle(program);
	}

	uint32_t VertexShader = glCreateShader(GL_VERTEX_SHADER);
	checkOpenGLError("glCreateShader<VERT> - %s", name().c_str());

	int nLen = static_cast<int>(vertex_source.size());
	const GLchar* src = vertex_source.c_str();

	glShaderSource(VertexShader, 1, &src, &nLen);
	checkOpenGLError("glShaderSource<VERT> - %s", name().c_str());

	glCompileShader(VertexShader);
	checkOpenGLError("glCompileShader<VERT> - %s", name().c_str());

	int vertStatus = GL_FALSE;
	int geomStatus = GL_FALSE;
	int fragStatus = GL_FALSE;
	
	GLsizei nLogLen = 0;
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &vertStatus);
	checkOpenGLError("glGetShaderiv<VERT> %s compile staus - %s", name().c_str());

	glGetShaderiv(VertexShader, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((vertStatus == GL_FALSE) && (nLogLen > 1))
	{
		DataStorage<GLchar> infoLog(nLogLen, 0);
		glGetShaderInfoLog(VertexShader, nLogLen, &nLogLen, infoLog.data());
		log::error("Vertex shader %s compile report:\n%s", name().c_str(), infoLog.data());
	}

	if (vertStatus == GL_TRUE)
	{
		glAttachShader(program, VertexShader);
		checkOpenGLError("glAttachShader<VERT> - %s", name().c_str());
	} 

	uint32_t GeometryShader = 0;

#	if defined(GL_GEOMETRY_SHADER)
	if ((geom_source.length() > 0) && (geom_source != etNoShader)) 
	{
		GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		checkOpenGLError("glCreateShader<GEOM> - %s", name().c_str());
		nLen = static_cast<int>(geom_source.size());
		src = geom_source.c_str();
		glShaderSource(GeometryShader, 1, &src, &nLen);
		checkOpenGLError("glShaderSource<GEOM> - %s", name().c_str());

		nLogLen = 0;
		glCompileShader(GeometryShader);
		checkOpenGLError("glCompileShader<GEOM> - %s", name().c_str());

		glGetShaderiv(GeometryShader, GL_COMPILE_STATUS, &geomStatus);
		checkOpenGLError("glGetShaderiv<GEOM> %s compile staus", name().c_str());
		
		glGetShaderiv(GeometryShader, GL_INFO_LOG_LENGTH, &nLogLen);
		if ((geomStatus == GL_FALSE) && (nLogLen > 1))
		{
			DataStorage<GLchar> infoLog(nLogLen, 0);
			glGetShaderInfoLog(GeometryShader, nLogLen, &nLogLen, infoLog.data());
			log::error("Geometry shader %s compile report:\n%s", name().c_str(), infoLog.data());
		}
		
		if (geomStatus == GL_TRUE)
		{
			glAttachShader(program, GeometryShader);
			checkOpenGLError("glAttachShader<GEOM> - %s", name().c_str());
		} 
	} 
#	endif

	///////////////////////////////////////////////// FRAGMENT
	uint32_t FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	checkOpenGLError("glCreateShader<FRAG> - %s", name().c_str());

	nLen = static_cast<int>(frag_source.size());
	src  = frag_source.c_str();

	glShaderSource(FragmentShader, 1, &src, &nLen);
	checkOpenGLError("glShaderSource<FRAG> - %s", name().c_str());

	glCompileShader(FragmentShader);
	checkOpenGLError("glCompileShader<FRAG> - %s", name().c_str());

	nLogLen = 0;
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &fragStatus);
	checkOpenGLError("glGetShaderiv<FRAG> %s compile staus ", name().c_str());

	glGetShaderiv(FragmentShader, GL_INFO_LOG_LENGTH, &nLogLen);
	if ((fragStatus == GL_FALSE) && (nLogLen > 1))
	{
		DataStorage<GLchar> infoLog(nLogLen, 0);
		glGetShaderInfoLog(FragmentShader, nLogLen, &nLogLen, infoLog.data());
		log::error("Fragment shader %s compile report:\n%s", name().c_str(), infoLog.data());
	}

	if (fragStatus == GL_TRUE)
	{
		glAttachShader(program, FragmentShader);
		checkOpenGLError("glAttachShader<FRAG> - %s", name().c_str());
	}

	int linkStatus = link();

	if (linkStatus == GL_TRUE)
	{
		int activeAttribs = 0;
		int maxNameLength = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttribs);
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
		for (uint32_t i = 0, e = static_cast<uint32_t>(activeAttribs); i < e; ++i)
		{ 
			int nameLength = 0;
			int attribSize = 0; 
			uint32_t attribType = 0;
			StringDataStorage name(maxNameLength, 0);
			glGetActiveAttrib(program, i, maxNameLength, &nameLength, &attribSize, &attribType, name.data());

			_attributes.emplace_back(name.data(), stringToVertexAttribute(name.data()));
		}

		for (auto& i : _attributes)
		{
			if ((i.usage != VertexAttributeUsage::InstanceId) && (i.usage != VertexAttributeUsage::InstanceIdExt))
			{
				glBindAttribLocation(static_cast<uint32_t>(apiHandle()), static_cast<GLuint>(i.usage), i.name.c_str());
				checkOpenGLError("glBindAttribLocation - %s", i.name.c_str());
			}
		}

#	if defined(GL_VERSION_3_0)
		if (glBindFragDataLocation != nullptr)
		{
			glBindFragDataLocation(program, 0, "FragColor0");
			checkOpenGLError("glBindFragDataLocation<color0> - %s", name().c_str());
			
			glBindFragDataLocation(program, 1, "FragColor1");
			checkOpenGLError("glBindFragDataLocation<color1> - %s", name().c_str());
			
			glBindFragDataLocation(program, 2, "FragColor2");
			checkOpenGLError("glBindFragDataLocation<color2> - %s", name().c_str());
			
			glBindFragDataLocation(program, 3, "FragColor3");
			checkOpenGLError("glBindFragDataLocation<color3> - %s", name().c_str());
			
			glBindFragDataLocation(program, 4, "FragColor4");
			checkOpenGLError("glBindFragDataLocation<color4> - %s", name().c_str());
		}
#	endif
		
		linkStatus = link();

		_rc->renderState().bindProgram(static_cast<uint32_t>(apiHandle()), true);

		if (linkStatus == GL_TRUE)
		{
			int activeUniforms = 0;
			_uniforms.clear();
			glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);
			glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
			for (uint32_t i = 0, e = static_cast<uint32_t>(activeUniforms); i < e; i++)
			{
				int uSize = 0;
				GLsizei uLenght = 0;
				StringDataStorage name(maxNameLength, 0);
				Program::Uniform P;
				glGetActiveUniform(program, i, maxNameLength, &uLenght, &uSize, &P.type, name.binary());
				P.location = glGetUniformLocation(program, name.binary());
				_uniforms[name.binary()] = P;

				if (strcmp(name.binary(), "mModelView") == 0)
					_mModelViewLocation = P.location;

				if (strcmp(name.binary(), "mModelViewProjection") == 0) 
					_mModelViewProjectionLocation = P.location;

				if (strcmp(name.binary(), "vCamera") == 0) 
					_vCameraLocation = P.location;

				if (strcmp(name.binary(), "vPrimaryLight") == 0) 
					_vPrimaryLightLocation = P.location;

				if (strcmp(name.binary(), "mLightProjectionMatrix") == 0) 
					_mLightProjectionMatrixLocation = P.location;

				if (strcmp(name.binary(), "mTransform") == 0)
					_mTransformLocation = P.location;
			}
		}
	}

	if (VertexShader != 0)
	{
		if (vertStatus == GL_TRUE)
		{
			glDetachShader(program, VertexShader);
			checkOpenGLError("Detach vertex shader");
		}

		glDeleteShader(VertexShader);
		checkOpenGLError("glDeleteShader(VertexShader)");
	}

	if (GeometryShader != 0)
	{
		if (geomStatus == GL_TRUE)
		{
			glDetachShader(program, GeometryShader);
			checkOpenGLError("Detache geometry shader");
		}
		
		glDeleteShader(GeometryShader);
		checkOpenGLError("glDeleteShader(GeometryShader)");
	}
	
	if (FragmentShader != 0)
	{
		if (fragStatus == GL_TRUE)
		{
			glDetachShader(program, FragmentShader);
			checkOpenGLError("Detach fragment shader");
		}
		
		glDeleteShader(FragmentShader);
		checkOpenGLError("glDeleteShader(FragmentShader)");
	}

	checkOpenGLError("Program::buildProgram -> end");
#endif
}

int Program::link()
{
	int result = 0;
	
#if !defined(ET_CONSOLE_APPLICATION)
	int nLogLen = 0;
	
	uint32_t program = static_cast<uint32_t>(apiHandle());

	glLinkProgram(program);
	checkOpenGLError("glLinkProgram - %s", name().c_str());

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	checkOpenGLError("glGetProgramiv<GL_LINK_STATUS> - %s", name().c_str());

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &nLogLen);
	checkOpenGLError("glGetProgramiv<GL_INFO_LOG_LENGTH> - %s", name().c_str());

	if (nLogLen > 1)
	{
		StringDataStorage infoLog(nLogLen + 1, 0);
		glGetProgramInfoLog(program, nLogLen, &nLogLen, infoLog.data());
		checkOpenGLError("glGetProgramInfoLog<LINK> - %s", name().c_str());
		
		if (result == GL_FALSE)
			log::error("Program %s link log:\n%s", name().c_str(), infoLog.data());
		else
			log::warning("Program %s link log:\n%s", name().c_str(), infoLog.data());
	}
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
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || (type == GL_SAMPLER_2D) || (type == GL_SAMPLER_CUBE));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, value);
	checkOpenGLError("glUniform1i");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, uint32_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || (type == GL_SAMPLER_2D) || (type == GL_SAMPLER_CUBE));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, int64_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || (type == GL_SAMPLER_2D) || (type == GL_SAMPLER_CUBE));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, uint64_t value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT((type == GL_INT) || (type == GL_SAMPLER_2D) || (type == GL_SAMPLER_CUBE));
	ET_ASSERT(apiHandleValid());

	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const unsigned long value, bool)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || (type == GL_SAMPLER_2D) || (type == GL_SAMPLER_CUBE));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const float value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_floatCache.count(nLoc) == 0) || (_floatCache[nLoc] != value)))
	{
		_floatCache[nLoc] = value;
		glUniform1f(nLoc, value);
		checkOpenGLError("glUniform1f");
	}
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec2& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC2);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_vec2Cache.count(nLoc) == 0) || (_vec2Cache[nLoc] != value)))
	{
		_vec2Cache[nLoc] = value;
		glUniform2fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform2fv");
	}
	
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec3& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC3);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_vec3Cache.count(nLoc) == 0) || (_vec3Cache[nLoc] != value)))
	{
		_vec3Cache[nLoc] = value;
		glUniform3fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform3fv");
	}
	
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec4& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_vec4Cache.count(nLoc) == 0) || (_vec4Cache[nLoc] != value)))
	{
		_vec4Cache[nLoc] = value;
		glUniform4fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform4fv");
	}
	
#endif
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const vec4& value)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandleValid());
	
	glUniform4fv(nLoc, 1, value.data());
	checkOpenGLError("glUniform4fv");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat3& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT3);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_mat3Cache.count(nLoc) == 0) || (_mat3Cache[nLoc] != value)))
	{
		_mat3Cache[nLoc] = value;
		glUniformMatrix3fv(nLoc, 1, 0, value.data());
		checkOpenGLError("glUniformMatrix3fv");
	}
	
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat4& value, bool forced)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandleValid());
	
	if (forced || ((_mat4Cache.count(nLoc) == 0) || (_mat4Cache[nLoc] != value)))
	{
		_mat4Cache[nLoc] = value;
		glUniformMatrix4fv(nLoc, 1, 0, value.data());
		checkOpenGLError("glUniformMatrix4fv");
	}
	
#endif
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const mat4& value)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandleValid());
	
	glUniformMatrix4fv(nLoc, 1, 0, value.data());
	checkOpenGLError("glUniformMatrix4fv");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec2* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC2);
	ET_ASSERT(apiHandleValid());
	
	glUniform2fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform2fv");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec3* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC3);
	ET_ASSERT(apiHandleValid());
	
	glUniform3fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform3fv");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const vec4* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandleValid());
	
	glUniform4fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform4fv");
#endif
}

void Program::setUniform(int nLoc, uint32_t type, const mat4* value, size_t amount)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandleValid());
	
	glUniformMatrix4fv(nLoc, static_cast<GLsizei>(amount), 0, value->data());
	checkOpenGLError("glUniformMatrix4fv");
#endif
}
