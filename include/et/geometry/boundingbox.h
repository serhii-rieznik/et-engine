/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	class BoundingBox
	{
	public:
		using Corners = std::array<vec3, 8>;
		
	public:
		BoundingBox() = default;
		
		BoundingBox(const vec3& aCenter, const vec3& aHalfDimension) :
			center(aCenter), halfDimension(aHalfDimension) { }
		
		void calculateCorners(BoundingBox::Corners& corners)
		{
			corners[0] = center + vec3(-halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[1] = center + vec3(halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[2] = center + vec3(-halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[3] = center + vec3(halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[4] = center + vec3(-halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[5] = center + vec3(halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[6] = center + vec3(-halfDimension.x, halfDimension.y, halfDimension.z);
			corners[7] = center + vec3(halfDimension.x, halfDimension.y, halfDimension.z);
		}

		void calculateTransformedCorners(BoundingBox::Corners& corners, const mat3& localTransform)
		{
			corners[0] = center + localTransform * vec3(-halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[1] = center + localTransform * vec3(halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[2] = center + localTransform * vec3(-halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[3] = center + localTransform * vec3(halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[4] = center + localTransform * vec3(-halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[5] = center + localTransform * vec3(halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[6] = center + localTransform * vec3(-halfDimension.x, halfDimension.y, halfDimension.z);
			corners[7] = center + localTransform * vec3(halfDimension.x, halfDimension.y, halfDimension.z);
		}
		
		vec3 minVertex() const
			{ return center - halfDimension; }

		vec3 maxVertex() const
			{ return center + halfDimension; }
		
	public:
		vec3 center = vec3(0.0f);
		vec3 halfDimension = vec3(0.0f);
	};
}
