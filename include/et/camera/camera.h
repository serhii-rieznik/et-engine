/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/frustum.h>

namespace et
{
	class Camera
	{
	public:
		Camera();

		void lookAt(const vec3& pos, const vec3& point = vec3(0.0f), const vec3& up = vec3(0.0f, 1.0f, 0.0f));
		
		const mat4& perspectiveProjection(float fov, float aspect, float zNear, float zFar);

		const mat4& customPerspectiveProjection(const vec2& fov, float zNear, float zFar);

		const mat4& orthogonalProjection(float left, float right, float top,
			float bottom, float zNear, float zFar);

		const mat4& windowProjection(const vec2& windowSize);

		const vec3& position() const
			{ return _inverseViewMatrix[3].xyz(); }

		void setPosition(const vec3& pos);

		const quaternion orientation() const
			{ return matrixToQuaternion(_viewMatrix.mat3()); }
		
		float zNear() const
			{ return _zNear; }
		
		float zFar() const
			{ return _zFar; }

		float perspectiveProjectionAspect() const
			{ return _perspecitveAspect; }

		vec3 direction() const;
		void setDirection(const vec3& d);
		void setSide(const vec3& s);

		const mat4& viewMatrix() const
			{ return _viewMatrix; }
		const mat4& inverseViewMatrix() const
			{ return _inverseViewMatrix; }

		const mat4& projectionMatrix() const 
			{ return _projectionMatrix; }
		const mat4& inverseProjectionMatrix() const
			{ return _inverseProjectionMatrix; }

		const mat4& viewProjectionMatrix() const
			{ return _viewProjectionMatrix; }
		const mat4& inverseViewProjectionMatrix() const
			{ return _inverseViewProjectionMatrix; }

		vec3 up() const;
		vec3 side() const;

		vec3 invDirection() const;
		vec3 invUp() const;
		vec3 invSide() const;

		float heading() const;

		void move(const vec3& dp);

		void rotate(const quaternion& q);
		void rotate(const vec3& axis);

		void lockUpVector(const vec3& u);
		void unlockUpVector();

		bool upVectorLocked() const
			{ return _lockUpVector; }

		const vec3& lockedUpVector() const 
			{ return _upLocked; }

		const Frustum& frustum() const
			{ return _frustum; }

		ray3d castRay(const vec2& pt) const;

		void setViewMatrix(const mat4& m)
			{ _viewMatrix = m; viewUpdated(); }
		
		void setProjectionMatrix(const mat4& m)
			{ _projectionMatrix = m; projectionUpdated(); }

		void reflected(const plane&, Camera&) const;

		vec3 project(const vec3&) const;
		vec3 unproject(const vec3&) const;

		vec4 project(const vec4&) const;
		vec4 unproject(const vec4&) const;

	private:
		void viewUpdated();
		void projectionUpdated();
		void updateViewProjectionMatrix();

	private:
		mat4 _viewMatrix;
		mat4 _inverseViewMatrix;
		
		mat4 _projectionMatrix;
		mat4 _inverseProjectionMatrix;
		
		mat4 _viewProjectionMatrix;
		mat4 _inverseViewProjectionMatrix;
		
		Frustum _frustum;

		vec3 _upLocked;
		float _zNear = 0.0f;
		float _zFar = 0.0f;
		float _perspecitveAspect = 0.0f;
		bool _lockUpVector = false;
	};

	typedef StaticDataStorage<mat4, 6> CubemapProjectionMatrixArray;
	CubemapProjectionMatrixArray cubemapMatrixProjectionArray(const mat4& proj, const vec3& point);
}
