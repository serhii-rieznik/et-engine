/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/opengl/opengl_program.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/opengl/opengl.h>
#include <et/app/application.h>
#include <et/camera/camera.h>

namespace et
{
OpenGLProgram::OpenGLProgram()
{
	initBuiltInUniforms();
}

OpenGLProgram::OpenGLProgram(const std::string& vertexShader, const std::string& geometryShader,
	const std::string& fragmentShader, const std::string& objName, const std::string& origin,
	const StringList& defines) : Program(objName, origin)
{
    setDefines(defines);
	initBuiltInUniforms();
	build(vertexShader, fragmentShader);
}

OpenGLProgram::~OpenGLProgram()
{
	uint32_t program = apiHandle();

	if ((program != 0) && glIsProgram(program))
	{
#	if (ET_EXPOSE_OLD_RENDER_STATE)
		_rc->renderState()->programDeleted(program);
#	endif
		glDeleteProgram(program);
		checkOpenGLError("glDeleteProgram: %s", name().c_str());
	}
}

void OpenGLProgram::bind()
{
	etUseProgram(apiHandle());
}

void OpenGLProgram::initBuiltInUniforms()
{
	_builtInUniforms["matWorld"] = &_matWorldLocation;
	_builtInUniforms["matView"] = &_matViewLocation;
	_builtInUniforms["matViewProjection"] = &_matViewProjectionLocation;
	_builtInUniforms["matLightViewProjection"] = &_matLightViewProjectionLocation;
	_builtInUniforms["defaultCamera"] = &_defaultCameraLocation;
	_builtInUniforms["defaultLight"] = &_defaultLightLocation;
}

int OpenGLProgram::getUniformLocation(const std::string& uniform) const
{
	ET_ASSERT(apiHandle() != 0);

	auto i = findConstant(uniform);
	return (i == shaderConstants().end()) ? -1 : i->second.location;
}

uint32_t OpenGLProgram::getUniformType(const std::string& uniform) const
{
	ET_ASSERT(apiHandle() != 0);

	auto i = findConstant(uniform);
	return (i == shaderConstants().end()) ? 0 : i->second.type;
}

Program::ShaderConstant OpenGLProgram::getUniform(const std::string& uniform) const
{
	ET_ASSERT(apiHandle() != 0);

	auto i = findConstant(uniform);
	return (i == shaderConstants().end()) ? Program::ShaderConstant() : i->second;
}

void OpenGLProgram::setViewMatrix(const mat4& m, bool forced)
{
	setUniform(_matViewLocation, GL_FLOAT_MAT4, m, forced);
}

void OpenGLProgram::setViewProjectionMatrix(const mat4& m, bool forced)
{
	setUniform(_matViewProjectionLocation, GL_FLOAT_MAT4, m, forced);
}

void OpenGLProgram::setCameraPosition(const vec3& p, bool forced)
{
	setUniform(_defaultCameraLocation, GL_FLOAT_VEC3, p, forced);
}

void OpenGLProgram::setDefaultLightPosition(const vec3& p, bool forced)
{
	setUniform(_defaultLightLocation, GL_FLOAT_VEC3, p, forced);
}

void OpenGLProgram::setLightProjectionMatrix(const mat4& m, bool forced)
{
	setUniform(_matLightViewProjectionLocation, GL_FLOAT_MAT4, m, forced);
}

void OpenGLProgram::setTransformMatrix(const mat4& m, bool forced)
{
	setUniform(_matWorldLocation, GL_FLOAT_MAT4, m, forced);
}

void OpenGLProgram::setCameraProperties(const Camera& cam)
{
	ET_ASSERT(apiHandle() != 0);

	setViewMatrix(cam.viewMatrix());
	setViewProjectionMatrix(cam.viewProjectionMatrix());
	setCameraPosition(cam.position());
}

void OpenGLProgram::printShaderSource(uint32_t shader, size_t initialSize, const char* tag)
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

void OpenGLProgram::printShaderLog(uint32_t shader, size_t initialSize, const char* tag)
{
	GLsizei nLogLen = 0;
	DataStorage<GLchar> infoLog(initialSize + 1, 0);
	glGetShaderInfoLog(shader, static_cast<GLsizei>(infoLog.size()), &nLogLen, infoLog.data());
	log::info("Program %s, %s shader info log:\n%s", name().c_str(), tag, infoLog.data());
}

void OpenGLProgram::build(const std::string& vertex_source, const std::string& frag_source)
{
	_floatCache.clear();
	_vec2Cache.clear();
	_vec3Cache.clear();
	_vec4Cache.clear();
	_mat3Cache.clear();
	_mat4Cache.clear();
    clearShaderConstants();
	
    std::string geom_source;
    
#if (ET_OPENGLES)
	if (!geom_source.empty())
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

	int nLen = static_cast<int32_t>(vertex_source.size());
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
	if (!geom_source.empty())
	{
		GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		checkOpenGLError("glCreateShader<GEOM> - %s", name().c_str());
		nLen = static_cast<int32_t>(geom_source.size());
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

	nLen = static_cast<int32_t>(frag_source.size());
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
		bind();

		if (linkStatus == GL_TRUE)
		{
			int activeUniforms = 0;
            clearShaderConstants();
			glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);
			glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
			for (uint32_t i = 0, e = static_cast<uint32_t>(activeUniforms); i < e; i++)
			{
				int uSize = 0;
				GLsizei uLenght = 0;
				StringDataStorage uniformName(maxNameLength + 1, 0);
				Program::ShaderConstant shaderConts;
				glGetActiveUniform(program, i, maxNameLength, &uLenght, &uSize, &shaderConts.type, uniformName.binary());
				shaderConts.location = glGetUniformLocation(program, uniformName.binary());
				
				std::string uniformNameString(uniformName.binary());
				shaderConstants().emplace(uniformNameString, shaderConts);

				auto builtIn = _builtInUniforms.find(uniformNameString);
				if (builtIn != _builtInUniforms.end())
				{
					*(builtIn->second) = shaderConts.location;
				}
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

	checkOpenGLError("OpenGLProgram::buildProgram -> end");
}

int OpenGLProgram::link(bool enableOutput)
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

bool OpenGLProgram::validate() const
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

void OpenGLProgram::setUniform(int nLoc, uint32_t type, int32_t value, bool)
{
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniformType(type));
	ET_ASSERT(apiHandle() != 0);
	
	glUniform1i(nLoc, value);
	checkOpenGLError("glUniform1i");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, uint32_t value, bool)
{
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniformType(type));
	ET_ASSERT(apiHandle() != 0);
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, int64_t value, bool)
{
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniformType(type));
	ET_ASSERT(apiHandle() != 0);
	
	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, uint64_t value, bool)
{
	(void)type;
	ET_ASSERT((type == GL_INT) || isSamplerUniformType(type));
	ET_ASSERT(apiHandle() != 0);

	glUniform1i(nLoc, static_cast<GLint>(value));
	checkOpenGLError("glUniform1i");
}

namespace
{
	template <typename C, typename V>
	inline bool shouldUpdateValue(const C& cache, int location, const V& value)
	{
		auto i = cache.find(location);
		return (i == cache.end()) || (i->second != value);
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const float value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_floatCache, nLoc, value))
	{
		_floatCache[nLoc] = value;
		glUniform1f(nLoc, value);
		checkOpenGLError("glUniform1f");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec2& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC2);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_vec2Cache, nLoc, value))
	{
		_vec2Cache[nLoc] = value;
		glUniform2fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform2fv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec3& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC3);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_vec3Cache, nLoc, value))
	{
		_vec3Cache[nLoc] = value;
		glUniform3fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform3fv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec4& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_vec4Cache, nLoc, value))
	{
		_vec4Cache[nLoc] = value;
		glUniform4fv(nLoc, 1, value.data());
		checkOpenGLError("glUniform4fv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec2i& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_INT_VEC2);
	ET_ASSERT(apiHandle() != 0);

	if (forced || shouldUpdateValue(_vec2iCache, nLoc, value))
	{
		_vec2iCache[nLoc] = value;
		glUniform2iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform2iv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec3i& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_INT_VEC3);
	ET_ASSERT(apiHandle() != 0);

	if (forced || shouldUpdateValue(_vec3iCache, nLoc, value))
	{
		_vec3iCache[nLoc] = value;
		glUniform3iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform3iv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec4i& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_INT_VEC4);
	ET_ASSERT(apiHandle() != 0);

	if (forced || shouldUpdateValue(_vec4iCache, nLoc, value))
	{
		_vec4iCache[nLoc] = value;
		glUniform4iv(nLoc, 1, value.data());
		checkOpenGLError("glUniform4iv");
	}
}

void OpenGLProgram::setUniformDirectly(int nLoc, uint32_t type, const vec4& value)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandle() != 0);
	
	glUniform4fv(nLoc, 1, value.data());
	checkOpenGLError("glUniform4fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const mat3& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT3);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_mat3Cache, nLoc, value))
	{
		_mat3Cache[nLoc] = value;
		glUniformMatrix3fv(nLoc, 1, 0, value.data());
		checkOpenGLError("glUniformMatrix3fv");
	}
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const mat4& value, bool forced)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandle() != 0);
	
	if (forced || shouldUpdateValue(_mat4Cache, nLoc, value))
	{
		_mat4Cache[nLoc] = value;
		glUniformMatrix4fv(nLoc, 1, 0, value.data());
		checkOpenGLError("glUniformMatrix4fv");
	}
}

void OpenGLProgram::setUniformDirectly(int nLoc, uint32_t type, const mat4& value)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandle() != 0);
	
	glUniformMatrix4fv(nLoc, 1, 0, value.data());
	checkOpenGLError("glUniformMatrix4fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const int* value, size_t amount)
{
	(void)type;
	ET_ASSERT(apiHandle() != 0);

	glUniform1iv(nLoc, static_cast<GLsizei>(amount), value);
	checkOpenGLError("glUniform1iv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t /* type */, const float* value, size_t amount)
{
	ET_ASSERT(apiHandle() != 0);

	glUniform1fv(nLoc, static_cast<GLsizei>(amount), value);
	checkOpenGLError("glUniform1fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec2* value, size_t amount)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC2);
	ET_ASSERT(apiHandle() != 0);
	
	glUniform2fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform2fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec3* value, size_t amount)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC3);
	ET_ASSERT(apiHandle() != 0);
	
	glUniform3fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform3fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const vec4* value, size_t amount)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_VEC4);
	ET_ASSERT(apiHandle() != 0);
	
	glUniform4fv(nLoc, static_cast<GLsizei>(amount), value->data());
	checkOpenGLError("glUniform4fv");
}

void OpenGLProgram::setUniform(int nLoc, uint32_t type, const mat4* value, size_t amount)
{
	(void)type;
	ET_ASSERT(type == GL_FLOAT_MAT4);
	ET_ASSERT(apiHandle() != 0);
	
	glUniformMatrix4fv(nLoc, static_cast<GLsizei>(amount), 0, value->data());
	checkOpenGLError("glUniformMatrix4fv");
}

void OpenGLProgram::setIntUniform(int location, const int* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform1iv(location, amount, data);
	checkOpenGLError("glUniform1iv");
}

void OpenGLProgram::setInt2Uniform(int location, const int* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform2iv(location, amount, data);
	checkOpenGLError("glUniform1iv");
}

void OpenGLProgram::setInt3Uniform(int location, const int* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform3iv(location, amount, data);
	checkOpenGLError("glUniform1iv");
}

void OpenGLProgram::setInt4Uniform(int location, const int* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform4iv(location, amount, data);
	checkOpenGLError("glUniform1iv");
}

void OpenGLProgram::setFloatUniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform1fv(location, amount, data);
	checkOpenGLError("glUniform1fv");
}

void OpenGLProgram::setFloat2Uniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform2fv(location, amount, data);
	checkOpenGLError("glUniform1fv");
}

void OpenGLProgram::setFloat3Uniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform3fv(location, amount, data);
	checkOpenGLError("glUniform1fv");
}

void OpenGLProgram::setFloat4Uniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniform4fv(location, amount, data);
	checkOpenGLError("glUniform1fv");
}

void OpenGLProgram::setMatrix3Uniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniformMatrix3fv(location, amount, false, data);
	checkOpenGLError("glUniformMatrix3fv");
}

void OpenGLProgram::setMatrix4Uniform(int location, const float* data, uint32_t amount)
{
	ET_ASSERT(apiHandle() != 0);
	glUniformMatrix4fv(location, amount, false, data);
	checkOpenGLError("glUniformMatrix4fv");
}

bool OpenGLProgram::isBuiltInUniformName(const std::string& name)
{
	return _builtInUniforms.count(name) > 0;
}

bool OpenGLProgram::isSamplerUniformType(uint32_t value)
{
	return
#if defined(GL_SAMPLER_1D)
	(value == GL_SAMPLER_1D) ||
#endif
	#if defined(GL_SAMPLER_3D)
	(value == GL_SAMPLER_3D) ||
#endif
#if defined(GL_SAMPLER_2D_RECT)
	(value == GL_SAMPLER_2D_RECT) ||
	(value == GL_SAMPLER_2D_RECT_SHADOW) ||
#endif
#if defined(GL_SAMPLER_2D_ARRAY)
	(value == GL_SAMPLER_2D_ARRAY) ||
	(value == GL_SAMPLER_2D_ARRAY_SHADOW) ||
#endif
#if defined(GL_SAMPLER_2D_SHADOW)
	(value == GL_SAMPLER_2D_SHADOW) ||
#endif
	(value == GL_SAMPLER_2D) || (value == GL_SAMPLER_CUBE);
}

DataType OpenGLProgram::uniformTypeToDataType(uint32_t t)
{
	if (isSamplerUniformType(t))
		return DataType::Int;
	
	return openglTypeToDataType(t);
}

const std::string& OpenGLProgram::commonHeader()
{
    static std::string _commonHeader;
    
    if (_commonHeader.empty())
    {
#if (ET_OPENGLES)
        _commonHeader =
        "#define etLowp         lowp\n"
        "#define etMediump		mediump\n"
        "#define etHighp		highp\n"
        "#define ET_OPENGL_ES	1\n";
        
        if (OpenGLCapabilities::instance().version() >= OpenGLVersion::Version_3x)
        {
            _commonHeader = "#version " + OpenGLCapabilities::instance().glslVersionShortString() + " es\n" +
            _commonHeader + "#define ET_OPENGL_ES_3\n";
        }
        else
        {
            _commonHeader += "#define ET_OPENGL_ES_2\n";
        }
        
#else
        _commonHeader = "#version " + OpenGLCapabilities::instance().glslVersionShortString() + "\n";
        
        if (OpenGLCapabilities::instance().glslVersionShortString() < "130")
            _commonHeader += "#extension GL_EXT_gpu_shader4 : enable\n";
        
        _commonHeader +=
        "#define etLowp\n"
        "#define etMediump\n"
        "#define etHighp\n";
#endif
    }
    return _commonHeader;
}
    
extern const std::string openGl2VertexHeader;
extern const std::string openGl3VertexHeader;

const std::string& OpenGLProgram::vertexShaderHeader()
{
    static std::string _vertShaderHeader;
    
    if (_vertShaderHeader.empty())
    {
        if (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x)
        {
            _vertShaderHeader = openGl2VertexHeader;
        }
        else
        {
            _vertShaderHeader = openGl3VertexHeader;
        }
    }
    
    return _vertShaderHeader;
}

extern const std::string openGl2FragmentHeader;
extern const std::string openGl3FragmentHeader;
 
const std::string& OpenGLProgram::fragmentShaderHeader()
{
    static std::string _fragShaderHeader;
    
    if (_fragShaderHeader.empty())
    {
        if (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x)
        {
            _fragShaderHeader = openGl2FragmentHeader;
        }
        else
        {
            _fragShaderHeader = openGl3FragmentHeader;
        }
    }
    
    return _fragShaderHeader;
}

const std::string openGl2VertexHeader = R"(
#define etShadow2D			shadow2D
#define etTexture2D			texture2D
#define etTextureCube		textureCube
#define etShadow2DProj		shadow2DProj
#define etTexture2DProj		texture2DProj
#define etTexture2DLod		texture2DLod
#define etTexture2DArray	texture2DArray
#define etTextureCubeLod	textureCubeLod
#define etTextureRect		texture2DRect
#define etVertexIn			attribute
#define etVertexOut			varying
#define VERTEX_SHADER 1
)";
    
const std::string openGl3VertexHeader = R"(
#define etShadow2D			texture
#define etTexture2D			texture
#define etTextureCube		texture
#define etTextureRect		texture
#define etTexture2DArray	texture
#define etShadow2DProj		textureProj
#define etTexture2DProj		textureProj
#define etTexture2DLod		textureLod
#define etTextureCubeLod	textureLod
#define etVertexIn			in
#define etVertexOut			out
#define VERTEX_SHADER 1
)";
    
const std::string openGl2FragmentHeader = R"(
#define etShadow2D			shadow2D
#define etTexture2D			texture2D
#define etTextureCube		textureCube
#define etShadow2DProj		shadow2DProj
#define etTexture2DProj		texture2DProj
#define etTexture2DLod		textureLod
#define etTexture2DArray	texture2DArray
#define etTextureCubeLod	textureCubeLod
#define etTextureRect		texture2DRect
#define etFragmentIn		varying
#define etFragmentOut		gl_FragColor
#define etFragmentOut0		gl_FragData[0]
#define etFragmentOut1		gl_FragData[1]
#define etFragmentOut2		gl_FragData[2]
#define etFragmentOut3		gl_FragData[3]
#define etFragmentOut4		gl_FragData[4]
#define etFragmentOut5		gl_FragData[5]
#define etFragmentOut6		gl_FragData[6]
#define etFragmentOut7		gl_FragData[7]
#define FRAGMENT_SHADER 1
)";
    
const std::string openGl3FragmentHeader = R"(
#define etShadow2D			texture
#define etTexture2D			texture
#define etTextureCube		texture
#define etTextureRect		texture
#define etTexture2DArray	texture
#define etShadow2DProj		textureProj
#define etTexture2DProj		textureProj
#define etTexture2DLod		textureLod
#define etTextureCubeLod	textureLod
#define etFragmentIn		in
#define etFragmentOut		etFragmentOut0
#define FRAGMENT_SHADER 1
layout (location = 0) out etHighp vec4 etFragmentOut0;
layout (location = 1) out etHighp vec4 etFragmentOut1;
layout (location = 2) out etHighp vec4 etFragmentOut2;
layout (location = 3) out etHighp vec4 etFragmentOut3;
layout (location = 4) out etHighp vec4 etFragmentOut4;
layout (location = 5) out etHighp vec4 etFragmentOut5;
layout (location = 6) out etHighp vec4 etFragmentOut6;
layout (location = 7) out etHighp vec4 etFragmentOut7;
)";
    
}
