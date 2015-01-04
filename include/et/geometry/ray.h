/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/line2d.h>

namespace et
{
	template <typename T>
	struct Ray3d
	{
	public:
		vector3<T> origin;
		vector3<T> direction;

	public:
		Ray3d()
			{ }
		
		Ray3d(const vector3<T>& o, const vector3<T>& d) :
			origin(o), direction(d) { }
	};

	template <typename T>
	struct Ray2d
	{
	public:
		vector2<T> origin;
		vector2<T> direction;
		
	public:
		Ray2d(const vector2<T>& o, const vector2<T>& d) :
			origin(o), direction(d) { }
		
		Line2d<T> line() const
			{ return Line2d<T>(origin, origin + direction); }
	};
}
