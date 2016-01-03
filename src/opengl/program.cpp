/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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

Program::Program(RenderContext* rc) :
	_rc(rc)
{
	
}

Program::Program(RenderContext* rc, const std::string& vertexShader, const std::string& geometryShader,
	const std::string& fragmentShader, const std::string& objName, const std::string& origin,
	const StringList& defines) : APIObject(objName, origin), _rc(rc), _defines(defines)
{
	buildProgram(vertexShader, geometryShader, fragmentShader);
}

Program::~Program()
{
	uint32_t program = apiHandle();
	if (program != 0)
	{
		_rc->renderState().programDeleted(program);
		if (glIsProgram(program))
		{
			glDeleteProgram(program);
			checkOpenGLError("glDeleteProgram: %s", name().c_str());
		}
	}
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

void Program::printShaderSource(uint32_t shader, size_t initialSize, const char* tag)
{
	GLsizei sourceLength = 0;
	DataStorage<GLchar> buffer(initialSize + 1, 0);
	glGetShaderSource(shader, static_cast<GLsizei>(buffer.size()), &sourceLength, buffer.data());
	
	DataStorage<char> stringBuffer(10 * sourceLength, 0);
	auto ptr = buffer.begin();
	auto end = ptr + sourceLength;
	
	int pos = 0;
	int lineNumber = 1;
	while (ptr < end)
	{
		pos += sprintf(stringBuffer.element_ptr(pos), "%04d > ", lineNumber);
		while ((ptr < end) && (*ptr != '\n'))
		{
			stringBuffer[pos++] = *ptr++;
		}
		stringBuffer[pos++] = '\n';
		++lineNumber;
		++ptr;
	}
	
	log::info("Program: %s, %s shader source:\n%s", name().c_str(), tag, stringBuffer.data());
}

void Program::printShaderLog(uint32_t shader, size_t initialSize, const char* tag)
{
	GLsizei nLogLen = 0;
	DataStorage<GLchar> infoLog(initialSize + 1, 0);
	glGetShaderInfoLog(shader, static_cast<GLsizei>(infoLog.size()), &nLogLen, infoLog.data());
	log::info("Program %s, %s shader info log:\n%s", name().c_str(), tag, infoLog.data());
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
	
#if (ET_OPENGLES)
	if (geom_source.length() && (geom_source != etNoShader))
		log::info("[Program] Geometry shader skipped in OpenGL ES");
#endif
	
	checkOpenGLError("[Program] buildProgram - %s", name().c_str());

	uint32_t program = apiHandle();
	
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
	checkOpenGLError("glGetShaderiv<VERTEX, GL_COMPILE_STATUS> - %s", name().c_str());

	glGetShaderiv(VertexShader, GL_INFO_LOG_LENGTH, &nLogLen);
	checkOpenGLError("glGetShaderiv<VERTEX, GL_INFO_LOG_LENGTH> - %s", name().c_str());
	if (nLogLen > 1)
	{
		printShaderLog(VertexShader, nLogLen, "vertex");
		printShaderSource(VertexShader, vertex_source.size(), "vertex");
	}

	if (vertStatus == GL_TRUE)
	{
		glAttachShader(program, VertexShader);
		checkOpenGLError("glAttachShader<VERT> - %s", name().c_str());
	} 

	uint32_t GeometryShader = 0;

#if defined(GL_GEOMETRY_SHADER)
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
		checkOpenGLError("glGetShaderiv<GEOM> %s compile status", name().c_str());
		
		glGetShaderiv(GeometryShader, GL_INFO_LOG_LENGTH, &nLogLen);
		if (nLogLen > 1)
		{
			printShaderLog(GeometryShader, geom_source.size(), "geometry");
			printShaderSource(GeometryShader, geom_source.size(), "geometry");
		}
		
		if (geomStatus == GL_TRUE)
		{
			glAttachShader(program, GeometryShader);
			checkOpenGLError("glAttachShader<GEOM> - %s", name().c_str());
		} 
	} 
#endif

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
	checkOpenGLError("glGetShaderiv<FRAG> %s compile status", name().c_str());

	glGetShaderiv(FragmentShader, GL_INFO_LOG_LENGTH, &nLogLen);
	if (nLogLen > 1)
	{
		printShaderLog(FragmentShader, frag_source.size(), "fragment");
		printShaderSource(FragmentShader, frag_source.size(), "fragment");
	}

	if (fragStatus == GL_TRUE)
	{
		glAttachShader(program, FragmentShader);
		checkOpenGLError("glAttachShader<FRAG> - %s", name().c_str());
	}

	int linkStatus = link(false);

	if (linkStatus == GL_TRUE)
	{
		int activeAttribs = 0;
		int maxNameLength = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &activeAttribs);
		checkOpenGLError("glGetProgramiv<GL_ACTIVE_ATTRIBUTES> - %s", name().c_str());

		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
		checkOpenGLError("glGetProgramiv<GL_ACTIVE_ATTRIBUTE_MAX_LENGTH> - %s", name().c_str());

		for (uint32_t i = 0, e = static_cast<uint32_t>(activeAttribs); i < e; ++i)
		{ 
			int nameLength = 0;
			int attribSize = 0; 
			uint32_t attribType = 0;
			StringDataStorage attribName(maxNameLength + 1, 0);

			glGetActiveAttrib(program, i, maxNameLength, &nameLength, &attribSize, &attribType, attribName.binary());
			checkOpenGLError("glGetActiveAttrib(..., %d, %d, ..., %s) - %s", maxNameLength, nameLength, 
				attribName.binary(), name().c_str());

			bool builtIn = false;
			auto attrib = stringToVertexAttributeUsage(attribName.binary(), builtIn);

			_attributes.emplace_back(std::string(attribName.binary()), attrib, builtIn);

			if (!builtIn)
			{
				glBindAttribLocation(apiHandle(), static_cast<GLuint>(attrib),
					attribName.binary());
				checkOpenGLError("glBindAttribLocation - %s", attribName.data());
			}
		}

		linkStatus = link(true);

		_rc->renderState().bindProgram(apiHandle(), true);

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
				StringDataStorage uniformName(maxNameLength + 1, 0);
				Program::Uniform P;
				glGetActiveUniform(program, i, maxNameLength, &uLenght, &uSize, &P.type, uniformName.binary());
				P.location = glGetUniformLocation(program, uniformName.binary());
				_uniforms[uniformName.binary()] = P;

				if (strcmp(uniformName.binary(), "mModelView") == 0)
					_mModelViewLocation = P.location;

				if (strcmp(uniformName.binary(), "mModelViewProjection") == 0)
					_mModelViewProjectionLocation = P.location;

				if (strcmp(uniformName.binary(), "vCamera") == 0)
					_vCameraLocation = P.location;

				if (strcmp(uniformName.binary(), "vPrimaryLight") == 0)
					_vPrimaryLightLocation = P.location;

				if (strcmp(uniformName.binary(), "mLightProjectionMatrix") == 0)
					_mLightProjectionMatrixLocation = P.location;

				if (strcmp(uniformName.binary(), "mTransform") == 0)
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
}

int Program::link(bool enableOutput)
{
	int result = 0;
	
	uint32_t program = apiHandle();

	const char* nameCStr = name().c_str();

	glLinkProgram(program);
	checkOpenGLError("glLinkProgram - %s", nameCStr);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	checkOpenGLError("glGetProgramiv<GL_LINK_STATUS> - %s", nameCStr);

	int nLogLen = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &nLogLen);
	checkOpenGLError("glGetProgramiv<GL_INFO_LOG_LENGTH> - %s", nameCStr);

	if ((nLogLen > 1) && (enableOutput || (result == GL_FALSE)))
	{
		StringDataStorage infoLog(nLogLen + 1, 0);

		glGetProgramInfoLog(program, nLogLen,  &nLogLen, infoLog.data());
		checkOpenGLError("glGetProgramInfoLog - %s, log length = %d", nameCStr, nLogLen);
		
		if (result == GL_FALSE)
			log::error("Program %s link log:\n%s", nameCStr, infoLog.data());
		else
			log::warning("Program %s link log:\n%s", nameCStr, infoLog.data());
	}

	return result;
}

bool Program::validate() const
{
	auto glId = static_cast<GLuint>(apiHandle());
	glValidateProgram(glId);
	checkOpenGLError("glValidateProgram");
	
	GLint result = 0;
	glGetProgramiv(glId, GL_VALIDATE_STATUS, &result);
	
	GLint nLogLen = 0;
	glGetProgramiv(glId, GL_INFO_LOG_LENGTH, &nLogLen);
	checkOpenGLError("glGetProgramiv<GL_INFO_LOG_LENGTH> - %s", name().c_str());
	
	if (nLogLen > 1)
	{
		StringDataStorage infoLog(nLogLen + 1, 0);
		glGetProgramInfoLog(glId, nLogLen, &nLogLen, infoLog.data());
		log::info("Program %s validation report:\n%s", name().c_str(), infoLog.data());
	}
	
	return (result == GL_TRUE);
}

/*
 * Uniform setters
 */

bool isSamplerUniform(uint32_t);

void Program::setUniform(int nLoc, uint32_t type, int32_t value, bool)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniform(type));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, value);
	checkOpenGLError("glUniform1i");
}

void Program::setUniform(int nLoc, uint32_t type, uint32_t value, bool)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniform(type));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void Program::setUniform(int nLoc, uint32_t type, int64_t value, bool)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniform(type));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void Program::setUniform(int nLoc, uint32_t type, uint64_t value, bool)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniform(type));
	ET_ASSERT(apiHandleValid());

	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void Program::setUniform(int nLoc, uint32_t type, const unsigned long value, bool)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniform(type));
	ET_ASSERT(apiHandleValid());
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void Program::setUniform(int nLoc, uint32_t type, const float value, bool forced)
{
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
}

void Program::setUniform(int nLoc, uint32_t type, const vec2& value, bool forced)
{
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
}

void Program::setUniform(int nLoc, uint32_t type, const vec3& value, bool forced)
{
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
}

void Program::setUniform(int nLoc, uint32_t type, const vec4& value, bool forced)
{
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
}

void Program::setUniform(int nLoc, uint32_t type, const vec2i& value, bool forced)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT(type == GL_INT_VEC2);
	ET_ASSERT(apiHandleValid());

	if (forced || ((_vec2iCache.count(nLoc) == 0) || (_vec2iCache[nLoc] != value)))
	{
		_vec2iCache[nLoc] = value;
		glUniform2iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform2iv");
	}
}

void Program::setUniform(int nLoc, uint32_t type, const vec3i& value, bool forced)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT(type == GL_INT_VEC3);
	ET_ASSERT(apiHandleValid());

	if (forced || ((_vec3iCache.count(nLoc) == 0) || (_vec3iCache[nLoc] != value)))
	{
		_vec3iCache[nLoc] = value;
		glUniform3iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform3iv");
	}
}

void Program::setUniform(int nLoc, uint32_t type, const vec4i& value, bool forced)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT(type == GL_INT_VEC4);
	ET_ASSERT(apiHandleValid());

	if (forced || ((_vec4iCache.count(nLoc) == 0) || (_vec4iCache[nLoc] != value)))
	{
		_vec4iCache[nLoc] = value;
		glUniform4iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform4iv");
	}
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const vec4& value)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandleValid());
	
	glUniform4fv(nLoc, 1, value.data());
	checkOpenGLError("glUniform4fv");
}

void Program::setUniform(int nLoc, uint32_t type, const mat3& value, bool forced)
{
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
}

void Program::setUniform(int nLoc, uint32_t type, const mat4& value, bool forced)
{
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
}

void Program::setUniformDirectly(int nLoc, uint32_t type, const mat4& value)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandleValid());
	
	glUniformMatrix4fv(nLoc, 1, 0, value.data());
	checkOpenGLError("glUniformMatrix4fv");
}

void Program::setUniform(int nLoc, uint32_t type, const int* value, size_t amount)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT(type == GL_INT);
	ET_ASSERT(apiHandleValid());

	glUniform1iv(nLoc, static_cast<GLsizei>(amount), value);
	checkOpenGLError("glUniform1iv");
}

void Program::setUniform(int nLoc, uint32_t type, const float* value, size_t amount)
{
	if (nLoc == -1) return;

	(void)type;
	ET_ASSERT(type == GL_FLOAT);
	ET_ASSERT(apiHandleValid());

	glUniform1fv(nLoc, static_cast<GLsizei>(amount), value);
	checkOpenGLError("glUniform1fv");
}

void Program::setUniform(int nLoc, uint32_t type, const vec2* value, size_t amount)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC2);
	ET_ASSERT(apiHandleValid());
	
	glUniform2fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform2fv");
}

void Program::setUniform(int nLoc, uint32_t type, const vec3* value, size_t amount)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC3);
	ET_ASSERT(apiHandleValid());
	
	glUniform3fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform3fv");
}

void Program::setUniform(int nLoc, uint32_t type, const vec4* value, size_t amount)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandleValid());
	
	glUniform4fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform4fv");
}

void Program::setUniform(int nLoc, uint32_t type, const mat4* value, size_t amount)
{
	if (nLoc == -1) return;
	
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandleValid());
	
	glUniformMatrix4fv(nLoc, static_cast<GLsizei>(amount), 0, value->data());
	checkOpenGLError("glUniformMatrix4fv");
}

/*
 * Service stuff
 */
bool isSamplerUniform(uint32_t value)
{
	return (value == GL_SAMPLER_2D) || (value == GL_SAMPLER_CUBE) || (value == GL_SAMPLER_2D_SHADOW) ||

#	if defined(GL_SAMPLER_1D)
		(value == GL_SAMPLER_1D) ||
#	endif

#	if defined(GL_SAMPLER_3D)
		(value == GL_SAMPLER_3D) ||
#	endif
	
#	if defined(GL_SAMPLER_2D_RECT)
		(value == GL_SAMPLER_2D_RECT) ||
		(value == GL_SAMPLER_2D_RECT_SHADOW) ||
#	endif
	
#	if defined(GL_SAMPLER_2D_ARRAY)
		(value == GL_SAMPLER_2D_ARRAY) ||
		(value == GL_SAMPLER_2D_ARRAY_SHADOW) ||
#	endif

		false;
}
