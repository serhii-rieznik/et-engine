/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	struct OBB
	{
		vec3 center;
		vec3 dimension;
		mat3 transform;

		OBB() 
			{ };

		OBB(const vec3& c, const vec3& d) : 
			center(c), dimension(d), transform(identityMatrix3) { }

		OBB(const vec3& c, const vec3& d, const mat3& t) : 
			center(c), dimension(d), transform(t) { }
	};
}
