/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>

namespace et
{
	class MatrixTransformable
	{ 
	public:
		MatrixTransformable() :
			_transform(identityMatrix) { }

		MatrixTransformable(const mat4& t) :
			_transform(t) { }

		virtual ~MatrixTransformable() { }

		mat4 transform() 
			{ return _transform; }

		const mat4& transform() const
			{ return _transform; }

		void setTransform(const mat4& transform)
			{ _transform = transform; }

		void applyTransform(const mat4& transform)
			{ _transform *= transform; }

	private:
		mat4 _transform;
	};

	class ComponentTransformable
	{
	public:
		ComponentTransformable() = default;
		virtual ~ComponentTransformable() { }

		const mat4& transform();
		const mat4& cachedTransform() const;
		const mat4& additionalTransform() const;
		
		void setTransform(const mat4& m);
		void setTransformDirectly(const mat4& m);
		
		void setAdditionalTransform(const mat4& m);

		void setTranslation(const vec3& t);
		void applyTranslation(const vec3& t);
		
		void setScale(const vec3& s);
		void applyScale(const vec3& s);
		
		void setOrientation(const quaternion& q);
		void applyOrientation(const quaternion& q);

		virtual void invalidateTransform();
		
		bool transformValid() const
			{ return (_flags & Flag_Valid); }
				
		const vec3& translation() const;
		const vec3& scale();
		const quaternion& orientation();

	private:
		enum Flags : size_t
		{
			Flag_Valid = 0x01,
			Flag_ShouldDecompose = 0x02,
		};

		bool shouldDecompose() const
			{ return (_flags & Flag_ShouldDecompose) != 0; }

		void buildTransform();

	private:
		mat4 _cachedTransform = identityMatrix;
		mat4 _additionalTransform = identityMatrix;
		
		vec3 _translation = vec3(0.0f);
		vec3 _scale = vec3(1.0f);
		quaternion _orientation;
		
		size_t _flags = 0;
	};
}
