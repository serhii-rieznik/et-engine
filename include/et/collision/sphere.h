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
	struct Sphere
	{
	public:
		Sphere() : 
			_radius(0.0f) { }

		Sphere(const vec3& aCenter, float aRadius) :
			_center(aCenter), _radius(aRadius) { } 

		vec3 center() 
			{ return _center; }

		const vec3& center() const
			{ return _center; }

		float radius() const
			{ return _radius; }
		
		void setRadius(float r)
			{ _radius = r; }

		void setCenter(const vec3& c)
			{ _center = c; }

		void applyTranslation(const vec3& dp)
			{ _center += dp; }

	protected:
		vec3 _center;
		float _radius;

	};
}
