/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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
