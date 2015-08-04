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
			vec4simd edge2to1;
			vec4simd edge3to1;
			size_t materialIndex = 0;

			float _dot00 = 0.0f;
			float _dot10 = 0.0f;
			float _dot01 = 0.0f;
			float _dot11 = 0.0f;
			float _invDenom = 0.0f;

			void computeSupportData()
			{
				edge2to1 = v[1] - v[0];
				edge3to1 = v[2] - v[0];
				_dot00 = edge2to1.dotSelf();
				_dot11 = edge3to1.dotSelf();
				_dot01 = edge2to1.dot(edge3to1);
				_invDenom = 1.0f / (_dot00 * _dot11 - _dot01 * _dot01);
			}

			vec4simd barycentric(vec4simd p) const
			{
				p -= v[0];
				float dot20 = p.dot(edge2to1);
				float dot21 = p.dot(edge3to1);
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
			bool sampled = false;
		};

		inline bool rayTriangle(const Ray& ray, const Triangle& tri, vec4simd& intersection_pt)
		{
			static const float epsilon = 0.000001f;

			vec4simd h = ray.direction.crossXYZ(tri.edge3to1);
			float a = tri.edge2to1.dot(h);
			if (a * a < epsilon)
				return false;

			vec4simd s = ray.origin - tri.v[0];
			float u = s.dot(h) / a;
			if (u * u > u)
				return false;

			vec4simd q = s.crossXYZ(tri.edge2to1);
			float v = ray.direction.dot(q) / a;
			if ((v < 0.0) || (u + v > 1.0))
				return false;

			float t = tri.edge3to1.dot(q) / a;
			intersection_pt = ray.origin + ray.direction * t;
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
			{ return v - n * (2.0f * v.dot(n)); }
	}
}
