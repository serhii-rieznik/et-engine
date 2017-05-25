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
	using Corners = std::array<vector3<float>, 8>;
	using Faces = std::array<Plane<float>, 6>;

public:
	BoundingBox() = default;

	BoundingBox(const vec3& aCenter, const vec3& aHalfDimension) :
		center(aCenter), halfDimension(aHalfDimension)
	{
	}

	void calculateCorners(BoundingBox::Corners& corners) const
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

	void calculateTransformedCorners(BoundingBox::Corners& corners, const mat3& localTransform) const
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

	BoundingBox transform(const mat4& t) const
	{
		Corners corners;
		calculateCorners(corners);
		vec3 vMin = vec3(std::numeric_limits<float>::max());
		vec3 vMax = -vMin;
		for (vec3& c : corners)
		{
			c = t * c;
			vMin = minv(vMin, c);
			vMax = maxv(vMax, c);
		}
		return BoundingBox(0.5f * (vMin + vMax), 0.5f * (vMax - vMin));
	}

	vec3 minVertex() const
	{
		return center - halfDimension;
	}

	vec3 maxVertex() const
	{
		return center + halfDimension;
	}

public:
	vec3 center = vec3(0.0f);
	vec3 halfDimension = vec3(0.0f);
};
}
