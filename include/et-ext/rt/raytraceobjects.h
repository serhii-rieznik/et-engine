/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector4-simd.h>
#include <et/scene3d/scene3d.h>
#include <random>

namespace et
{
namespace rt
{
#define ET_RT_EVALUATE_DISTRIBUTION				0
#define ET_RT_EVALUATE_SAMPLER					0
#define ET_RT_VISUALIZE_BRDF					0
#define ET_RT_USE_MT_GENERATOR					1

using float3 = vector3<float>;
using float4 = vec4simd;

enum : uint32_t
{
	InvalidIndex = uint32_t(-1),
	MaxAxisIndex = 2
};

struct Constants
{
	static const float epsilon;
	static const float initialSplitValue;
};

enum class SamplingMethod : uint32_t
{
	Uniform,
	Cosine,
	RaisedCosine
};

enum class RaytraceMethod : uint32_t
{
	BackwardPathTracing,
	ForwardLightTracing
};

struct Options
{
	uint32_t threads = 0;
	uint32_t raysPerPixel = 32;
	uint32_t maxPathLength = 0;
	uint32_t maxKDTreeDepth = 32;
	uint32_t renderRegionSize = 32;
	uint32_t lightSamples = 1;
	uint32_t bsdfSamples = 1;
	float apertureSize = 0.0f;
	float focalDistanceCorrection = 0.0f;
	RaytraceMethod method = RaytraceMethod::BackwardPathTracing;
	bool renderKDTree = false;
};

struct ET_ALIGNED(16) Triangle
{
	float4 v[3];
	float4 n[3];
	float4 t[3];
	float4 edge1to0;
	float4 edge2to0;
	float4 _invDenom;
	uint32_t materialIndex = 0;

	void computeSupportData()
	{
		edge1to0 = v[1] - v[0];
		edge2to0 = v[2] - v[0];
		_dot00 = edge1to0.dotSelf();
		_dot11 = edge2to0.dotSelf();
		_dot01 = edge1to0.dot(edge2to0);
		_invDenomValue = 1.0f / (_dot00 * _dot11 - _dot01 * _dot01);
		_invDenom = rt::float4(_invDenomValue);
		_area = 0.5f * edge1to0.crossXYZ(edge2to0).length();
	}

	float4 barycentric(const float4& inP) const
	{
		float4 p = inP - v[0];
		float dot20 = p.dot(edge1to0);
		float dot21 = p.dot(edge2to0);
		float y = _dot11 * dot20 - _dot01 * dot21;
		float z = _dot00 * dot21 - _dot01 * dot20;
		return float4(_invDenomValue - y - z, y, z, 0.0f) * _invDenom;
	}

	float4 interpolatedPosition(const float4& b) const
	{
		return v[0] * b.shuffle<0, 0, 0, 3>() + v[1] * b.shuffle<1, 1, 1, 3>() + v[2] * b.shuffle<2, 2, 2, 3>();
	}

	float4 interpolatedNormal(const float4& b) const
	{
		auto result =
			n[0] * b.shuffle<0, 0, 0, 3>() +
			n[1] * b.shuffle<1, 1, 1, 3>() +
			n[2] * b.shuffle<2, 2, 2, 3>();
		result.normalize();
		return result;
	}

	float4 interpolatedTexCoord0(const float4& b) const
	{
		auto result =
			t[0] * b.shuffle<0, 0, 0, 3>() +
			t[1] * b.shuffle<1, 1, 1, 3>() +
			t[2] * b.shuffle<2, 2, 2, 3>();
		result.normalize();
		return result;
	}

	float4 geometricNormal() const
	{
		float4 c = edge1to0.crossXYZ(edge2to0);
		c.normalize();
		return c;
	}

	float4 averageNormal() const
	{
		float4 result = n[0] + n[1] + n[2];
		result.normalize();
		return result;
	}

	float4 minVertex() const
	{
		return v[0].minWith(v[1].minWith(v[2]));
	}

	float4 maxVertex() const
	{
		return v[0].maxWith(v[1].maxWith(v[2]));
	}

	float area() const
	{
		return _area;
	}

private:
	float _invDenomValue = 0.0f;
	float _dot00 = 0.0f;
	float _dot01 = 0.0f;
	float _dot11 = 0.0f;
	float _area = 0.0f;
};
using TriangleList = Vector<rt::Triangle>;

struct ET_ALIGNED(16) IntersectionData
{
	float4 v0;
	float4 edge1to0;
	float4 edge2to0;

	IntersectionData(const float4& v, const float4& e1, const float4& e2) :
		v0(v), edge1to0(e1), edge2to0(e2)
	{
	}
};

struct ET_ALIGNED(16) BoundingBox
{
	float4 center = float4(0.0f);
	float4 halfSize = float4(0.0f);

	BoundingBox() = default;

	BoundingBox(const float4& c, const float4& hs) :
		center(c), halfSize(hs)
	{
	}

	BoundingBox(const float4& minVertex, const float4& maxVertex, int)
	{
		center = (minVertex + maxVertex) * 0.5f;
		halfSize = (maxVertex - minVertex) * 0.5f;
	}

	float4 minVertex() const
	{
		return center - halfSize;
	}

	float4 maxVertex() const
	{
		return center + halfSize;
	}

	float square() const
	{
		float xy = halfSize.cX() * halfSize.cY();
		float yz = halfSize.cY() * halfSize.cZ();
		float xz = halfSize.cX() * halfSize.cZ();
		return 4.0f * (xy + yz + xz);
	}

	float volume() const
	{
		return 8.0f * halfSize.cX() * halfSize.cY() * halfSize.cZ();
	};
};

struct ET_ALIGNED(16) Ray
{
	float4 origin;
	float4 direction;

	Ray() = default;

	Ray(const float4& o, const float4& d) :
		origin(o), direction(d)
	{
	}

	Ray(const ray3d& r) :
		origin(r.origin, 1.0f), direction(r.direction, 0.0f)
	{
	}
};

struct Region
{
	vec2i origin = vec2i(0);
	vec2i size = vec2i(0);
	size_t estimatedBounces = 0;
	bool sampled = false;
};

inline float fastRandomFloat()
{
#if (ET_RT_USE_MT_GENERATOR)
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
	return dis(gen);
#else
	static uint32_t seed = static_cast<uint32_t>(time(nullptr));
	union
	{
		uint32_t u;
		float f;
	} wrap = { ((seed *= 16807) >> 9) | 0x3f800000 };
	return wrap.f - 1.0f;
#endif
}

inline float4 normalize(float4 n)
{
	n.normalize();
	return n;
}

inline float4 perpendicularVector(const float4& normal)
{
	auto componentsLength = (normal * normal).xyz();

	if (componentsLength.x > 0.5f)
	{
		float scaleFactor = std::sqrt(componentsLength.z + componentsLength.x);
		return normal.shuffle<2, 3, 0, 1>() * float4(1.0f / scaleFactor, 0.0f, -1.0f / scaleFactor, 0.0f);
	}
	else if (componentsLength.y > 0.5f)
	{
		float scaleFactor = std::sqrt(componentsLength.y + componentsLength.x);
		return normal.shuffle<1, 0, 3, 3>() * float4(-1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f, 0.0f);
	}
	float scaleFactor = std::sqrt(componentsLength.z + componentsLength.y);
	return normal.shuffle<3, 2, 1, 3>() * float4(0.0f, -1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f);
}

inline float uniformDistribution(float Xi, ...)
{
	return Xi;
}

inline float cosineDistribution(float Xi, ...)
{
	return std::sqrt(Xi);
}

inline float ggxDistribution(float Xi, float alpha)
{
	return std::sqrt((1.0f - Xi) / ((sqr(alpha) - 1.0f) * Xi + 1.0f));
}

template <typename F, typename ... Arg>
inline float4 randomVectorOnHemisphere(const float4& rnd, const float4& normal, F distribution, Arg... args)
{
	float Xi = rnd.component<0>();
	float phi = rnd.component<1>() * DOUBLE_PI;
	float cosTheta = distribution(Xi, std::forward<Arg>(args)...);
	float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
	float4 u = perpendicularVector(normal);
	float4 v = u.crossXYZ(normal);
	return (u * std::cos(phi) + v * std::sin(phi)) * sinTheta + normal * cosTheta;
}

inline float4 reflect(const float4& v, const float4& n)
{
	const float4 two(2.0f);
	return v - two * n * v.dotVector(n);
}

bool rayToBoundingBox(Ray r, BoundingBox box, float& tNear, float& tFar);

inline float fresnelShlickApproximation(float f0, float cosTheta)
{
	return f0 + (1.0f - f0) * std::pow(1.0f - std::abs(cosTheta), 5.0f);
}

inline float4 randomBarycentric()
{
	float r1 = std::sqrt(fastRandomFloat());
	float r2 = fastRandomFloat();
	return float4(1.0f - r1, r1 * (1.0f - r2), r1 * r2, 0.0f);
}

const float4& defaultLightDirection();

float4 computeDiffuseVector(const float4& i, const float4& n, float r);
float4 computeReflectionVector(const float4& i, const float4& n, float r);
float4 computeRefractionVector(const float4& i, const float4& n, float eta, float r, float sinTheta, float IdotN);
}
}
