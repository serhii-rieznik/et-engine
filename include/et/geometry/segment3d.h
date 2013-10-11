/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/vector3.h>

namespace et
{
	template <typename T>
	struct Segment3d
	{
		vector3<T> start;
		vector3<T> end;

	public:
		Segment3d(const vector3<T>& s, const vector3<T>& e) :
			start(s), end(e) { }

		vector3<T> direction() const 
			{ return normalize(end - start); }
	};
}