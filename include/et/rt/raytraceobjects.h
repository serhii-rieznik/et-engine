/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector4-simd.h>

namespace et
{
	namespace rt
	{
		struct Material
		{
			std::string name;
			vec4simd diffuse;
			vec4simd specular;
			vec4simd emissive;
			float roughness = 0.0f;
		};

		struct Triangle
		{
			vec4simd v[3];
			vec4simd n[3];
			vec4simd edge1to0;
			vec4simd edge2to0;
			size_t materialIndex = 0;

			float _dot00 = 0.0f;
			float _dot10 = 0.0f;
			float _dot01 = 0.0f;
			float _dot11 = 0.0f;
			float _invDenom = 0.0f;

			void computeSupportData()
			{
				edge1to0 = v[1] - v[0];
				edge2to0 = v[2] - v[0];
				_dot00 = edge1to0.dotSelf();
				_dot11 = edge2to0.dotSelf();
				_dot01 = edge1to0.dot(edge2to0);
				_invDenom = 1.0f / (_dot00 * _dot11 - _dot01 * _dot01);
			}

			vec4simd barycentric(vec4simd p) const
			{
				p -= v[0];
				float dot20 = p.dot(edge1to0);
				float dot21 = p.dot(edge2to0);
				float y = (_dot11 * dot20 - _dot01 * dot21) * _invDenom;
				float z = (_dot00 * dot21 - _dot01 * dot20) * _invDenom;
				float x = 1.0f - y - z;
				return vec4simd(x, y, z, 0.0f);
			}

			vec4simd interpolatedNormal(const vec4simd& b) const
			{
				return n[0] * b.shuffle<0, 0, 0, 0>() + 
					n[1] * b.shuffle<1, 1, 1, 1>() + n[2] * b.shuffle<2, 2, 2, 2>();
			}
		};

		struct Ray
		{
			vec4simd origin;
			vec4simd direction;

			Ray() { }

			Ray(const vec4simd& o, const vec4simd& d) : 
				origin(o), direction(d) { }

			Ray(const ray3d& r) : 
				origin(r.origin, 1.0f), direction(r.direction, 0.0f) { } 
		};

		struct Region
		{
			vec2i origin = vec2i(0);
			vec2i size = vec2i(0);
			size_t estimatedBounces = 0;
			bool sampled = false;
		};

		inline bool rayTriangle(const Ray& ray, const Triangle& tri, vec4simd& intersection_pt, vec4simd& barycentric)
		{
			const float epsilon = 0.0005f;
			const float minusEpsilon = -0.5f * epsilon;
			const float onePlusEpsilon = 1.0f + 0.5f * epsilon;
			const float epsilonSquared = epsilon * epsilon;

			vec4simd pvec = ray.direction.crossXYZ(tri.edge2to0);
			float det = tri.edge1to0.dot(pvec);
			if (det * det < epsilonSquared)
				return false;

			vec4simd tvec = ray.origin - tri.v[0];
			float u = tvec.dot(pvec) / det;
			if ((u < minusEpsilon) || (u > onePlusEpsilon))
				return false;

			vec4simd qvec = tvec.crossXYZ(tri.edge1to0);
			float v = ray.direction.dot(qvec) / det;
			if ((v < minusEpsilon) || (u + v > onePlusEpsilon))
				return false;

			float t = tri.edge2to0.dot(qvec) / det;
			intersection_pt = ray.origin + ray.direction * t;
			barycentric = vec4simd(1.0f - u - v, u, v, 0.0f);
			return (t > epsilon);
		}

		vec4simd perpendicularVector(const vec4simd& normal)
		{
			vec3 componentsLength = (normal * normal).xyz();

			if (componentsLength.x > 0.5f)
			{
				float scaleFactor = std::sqrt(componentsLength.z + componentsLength.x);
				return normal.shuffle<2, 3, 0, 1>() * vec4simd(1.0f / scaleFactor, 0.0f, -1.0f / scaleFactor, 0.0f);
			}
			else if (componentsLength.y > 0.5f)
			{
				float scaleFactor = std::sqrt(componentsLength.y + componentsLength.x);
				return normal.shuffle<1, 0, 3, 3>() * vec4simd(-1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f, 0.0f);
			}
			float scaleFactor = std::sqrt(componentsLength.z + componentsLength.y);
			return normal.shuffle<3, 2, 1, 3>() * vec4simd(0.0f, -1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f);
		}

		inline vec4simd randomVectorOnHemisphere(const vec4simd& normal, float distributionAngle)
		{
			float phi = randomFloat() * DOUBLE_PI;
			float theta = std::sin(randomFloat() * clamp(distributionAngle, 0.0f, HALF_PI));
			vec4simd u = perpendicularVector(normal);
			vec4simd result = (u * std::cos(phi) + u.crossXYZ(normal) * std::sin(phi)) * std::sqrt(theta) +
				normal * std::sqrt(1.0f - theta);
			result.normalize();
			return result;
		}

		inline vec4simd reflect(const vec4simd& v, const vec4simd& n)
		{
			const vec4simd two(2.0f);
			return v - two * n * v.dotVector(n);
		}
	}
}
