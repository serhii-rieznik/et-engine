/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/object.h>
#include <et/opengl/opengltypes.h>

namespace et
{
	struct ProgramUniform
	{
		uint32_t type;
		int location;

		ProgramUniform() :
			type(0), location(-1) { }
	};

	struct ProgramAttrib
	{
		std::string name;
		VertexAttributeUsage usage;
		
		ProgramAttrib(const std::string& aName, VertexAttributeUsage aUsage) :
			name(aName), usage(aUsage) { }
	};

	typedef std::map<std::string, ProgramUniform> UniformMap;

	class Camera;
	class RenderState;
	
	class Program : public LoadableObject
	{
	public:
		ET_DECLARE_POINTER(Program)

	public:
		Program(RenderState& rs);
		
		Program(RenderState& rs, const std::string& vertexShader, const std::string& geometryShader,
			const std::string& fragmentShader, const std::string& objName, const std::string& origin,
			const StringList& defines);

		~Program();

		int getUniformLocation(const std::string& uniform) const;
		uint32_t getUniformType(const std::string& uniform) const;
		ProgramUniform getUniform(const std::string& uniform) const;

		void validate() const;

		uint32_t glID() const
			{ return _glID; }
		
		bool loaded() const
			{ return _glID != 0; }

		int modelViewMatrixUniformLocation() const 
			{ return _mModelViewLocation; }

		int mvpMatrixUniformLocation() const
			{ return _mModelViewProjectionLocation; }

		int cameraUniformLocation() const
			{ return _vCameraLocation; }

		int primaryLightUniformLocation() const
			{ return _vPrimaryLightLocation; }

		int lightProjectionMatrixLocation() const
			{ return _mLightProjectionMatrixLocation; }

		int transformMatrixLocation() const
			{ return _mTransformLocation; }

		void setModelViewMatrix(const mat4 &m, bool force = false);
		void setMVPMatrix(const mat4 &m, bool force = false);
		void setCameraPosition(const vec3& p, bool force = false);
		void setPrimaryLightPosition(const vec3& p, bool force = false);
		void setLightProjectionMatrix(const mat4 &m, bool force = false);
		void setTransformMatrix(const mat4 &m, bool force = false);

		void setCameraProperties(const Camera& cam);

		void setUniform(int nLoc, uint32_t, const int value);
		void setUniform(int nLoc, uint32_t, const unsigned int value);
		void setUniform(int nLoc, uint32_t, const unsigned long value);
		
		void setUniform(int nLoc, uint32_t, const float value, bool force = false);
		void setUniform(int nLoc, uint32_t, const vec2& value, bool force = false);
		void setUniform(int nLoc, uint32_t, const vec3& value, bool force = false);
		void setUniform(int nLoc, uint32_t, const vec4& value, bool force = false);
		void setUniform(int nLoc, uint32_t, const mat3& value, bool force = false);
		void setUniform(int nLoc, uint32_t, const mat4& value, bool force = false);
		
		template <typename T>
		void setUniform(const std::string& name, const T& value)
		{
			auto i = findUniform(name);
			if (i != _uniforms.end())
				setUniform(i->second.location, i->second.type, value);
		}

		template <typename T>
		void setUniform(const ProgramUniform& u, const T& value)
			{ setUniform(u.location, u.type, value); }
		
		void buildProgram(const std::string& vertex_source, const std::string& geom_source,
			const std::string& frag_source);
		
		const StringList& defines() const
			{ return _defines; }

	private:
		UniformMap::const_iterator findUniform(const std::string& name) const;
		
		int link();

	private:
		RenderState& _rs;

		uint32_t _glID;
		
		UniformMap _uniforms;
		std::vector<ProgramAttrib> _attributes;

		int _mModelViewLocation;
		int _mModelViewProjectionLocation;
		int _vCameraLocation;
		int _vPrimaryLightLocation;
		int _mLightProjectionMatrixLocation;
		int _mTransformLocation;

		std::map<int, float> _floatCache;
		std::map<int, vec2> _vec2Cache;
		std::map<int, vec3> _vec3Cache;
		std::map<int, vec4> _vec4Cache;
		std::map<int, mat3> _mat3Cache;
		std::map<int, mat4> _mat4Cache;
		
		StringList _defines;
	};
}
