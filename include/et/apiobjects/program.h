/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <unordered_map>
#include <et/rendering/rendering.h>

namespace et
{
	class Camera;
	class RenderState;
	
	class Program : public LoadableObject
	{
	public:
		ET_DECLARE_POINTER(Program)

		struct Uniform
		{
			uint32_t type = 0;
			int location = -1;
		};
		
		struct Attribute
		{
			std::string name;
			VertexAttributeUsage usage = VertexAttributeUsage::Position;
			
			Attribute(const std::string& aName, VertexAttributeUsage aUsage) :
				name(aName), usage(aUsage) { }
		};
		
		typedef std::unordered_map<std::string, Uniform> UniformMap;
		
	public:
		Program(RenderContext*);
		
		Program(RenderContext*, const std::string& vertexShader, const std::string& geometryShader,
			const std::string& fragmentShader, const std::string& objName, const std::string& origin,
			const StringList& defines);

		~Program();

		int getUniformLocation(const std::string& uniform) const;
		uint32_t getUniformType(const std::string& uniform) const;
		Program::Uniform getUniform(const std::string& uniform) const;

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

		const Program::UniformMap& uniforms() const 
			{ return _uniforms; }

		void setUniform(int, uint32_t, const int32_t, bool);
		void setUniform(int, uint32_t, const uint32_t, bool);
		void setUniform(int, uint32_t, const int64_t, bool);
		void setUniform(int, uint32_t, const uint64_t, bool);
		void setUniform(int, uint32_t, const unsigned long, bool);
		
		void setUniform(int, uint32_t, const float, bool force = false);
		
		void setUniform(int, uint32_t, const vec2&, bool force = false);
		void setUniform(int, uint32_t, const vec3&, bool force = false);
		void setUniform(int, uint32_t, const vec4&, bool force = false);
		
		void setUniform(int, uint32_t, const mat3&, bool force = false);
		void setUniform(int, uint32_t, const mat4&, bool force = false);

		void setUniform(int, uint32_t, const vec2* value, size_t amount);
		void setUniform(int, uint32_t, const vec3* value, size_t amount);
		void setUniform(int, uint32_t, const vec4* value, size_t amount);
		void setUniform(int, uint32_t, const mat4* value, size_t amount);

		void setUniformDirectly(int, uint32_t, const vec4&);
		void setUniformDirectly(int, uint32_t, const mat4& value);
		
		template <typename T>
		void setUniform(const std::string& name, const T& value, bool force = false)
		{
			auto i = findUniform(name);
			if (i != _uniforms.end())
				setUniform(i->second.location, i->second.type, value, force);
		}

		template <typename T>
		void setUniform(const std::string& name, const T* value, size_t amount)
		{
			auto i = findUniform(name);
			if (i != _uniforms.end())
				setUniform(i->second.location, i->second.type, value, amount);
		}
		
		template <typename T>
		void setUniform(const Program::Uniform& u, const T& value, bool force = false)
			{ setUniform(u.location, u.type, value, force); }

		template <typename T>
		void setUniformDirectly(const Program::Uniform& u, const T& value)
			{ setUniformDirectly(u.location, u.type, value); }
		
		template <typename T>
		void setUniform(const Program::Uniform& u, const T* value, size_t amount)
			{ setUniform(u.location, u.type, value, amount); }
		
		void buildProgram(const std::string& vertex_source, const std::string& geom_source,
			const std::string& frag_source);
		
		const StringList& defines() const
			{ return _defines; }

	private:
		Program::UniformMap::const_iterator findUniform(const std::string& name) const;
		
		int link();

	private:
		RenderContext* _rc;
		
		uint32_t _glID;
		
		Program::UniformMap _uniforms;
		std::vector<Attribute> _attributes;

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
