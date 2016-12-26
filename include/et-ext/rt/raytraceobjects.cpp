/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/raytrace.h>
#include <et-ext/rt/raytraceobjects.h>

namespace et {
namespace rt {

const float_type Constants::epsilon = 0.0001f;
const float_type Constants::distanceEpsilon = 20.0f * Constants::epsilon;
const float_type Constants::initialSplitValue = std::numeric_limits<float>::max();

const float4& defaultLightDirection()
{
	static float4 d(0.0f, 1.0f, 1.0f, 0.0f);
	static bool shouldNormalize = true;
	if (shouldNormalize)
	{
		d.normalize();
		shouldNormalize = false;
	}
	return d;
}

float4 computeDiffuseVector(const float4& incidence, const float4& normal, float roughness)
{
#if (ET_RT_VISUALIZE_BRDF)
    return defaultLightDirection();
#else
	return randomVectorOnHemisphere(normal, cosineDistribution);
#endif
}

float4 computeReflectionVector(const float4& incidence, const float4& normal, float roughness)
{
#if (ET_RT_VISUALIZE_BRDF)
    return defaultLightDirection();
#else
    float4 idealReflection = reflect(incidence, normal);
	
#	define MAX_REFLECTION_ATTEMPTS 16

	uint32_t attempts = 0;
	auto result = randomVectorOnHemisphere(idealReflection, ggxDistribution, roughness);
	while ((result.dot(normal) <= 0.0f) && (attempts < MAX_REFLECTION_ATTEMPTS))
	{
		result = randomVectorOnHemisphere(idealReflection, ggxDistribution, roughness);
		++attempts;
	}

	return result;
#endif
}

float4 computeRefractionVector(const float4& Wi, const float4& n, float_type eta, float roughness, float sinTheta, float IdotN)
{
#	define MAX_REFRACTION_ATTEMPTS 16

	ET_ASSERT(sinTheta > 0);

	float4 idealRefraction = Wi * eta - n * (eta * IdotN + std::sqrt(sinTheta));
	float4 result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);

	uint32_t attempts = 0;
	while ((result.dot(n) >= 0.0f) && (attempts < MAX_REFRACTION_ATTEMPTS))
	{
		result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);
		++attempts;
	}
	return randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);
}

#define SIGN_MASK 0x80000000
#define SIGN_SHIFT 29

bool rayToBoundingBox(Ray r, BoundingBox box, float& tNear, float& tFar)
{
	ET_ALIGNED(16) union { float a; uint32_t i; } diff;
	ET_ALIGNED(16) union { float f[4]; uint32_t i[4]; } dir;
	ET_ALIGNED(16) float_type bs[8];

	(r.direction & SIGN_MASK).loadToFloats(dir.f);
	((box.minVertex() - r.origin) / r.direction).loadToFloats(bs);
	((box.maxVertex() - r.origin) / r.direction).loadToFloats(bs + 4);

	uint32_t xs = dir.i[0] >> SIGN_SHIFT;
	uint32_t ys = dir.i[1] >> SIGN_SHIFT;

	float bsy1 = bs[5 - ys];
	tNear = bs[xs];
	diff = { bsy1 - tNear };
	if (diff.i & SIGN_MASK)
		return false;

	float bsy0 = bs[ys + 1];
	tFar = bs[4 - xs];
	diff = { tFar - bsy0 };
    if (diff.i & SIGN_MASK)
        return false;

	diff = { tNear - bsy0 };
	if (diff.i & SIGN_MASK)
        tNear = bsy0;

	uint32_t zs = dir.i[2] >> SIGN_SHIFT;
	diff = { bs[6 - zs] - tNear };
	if (diff.i & SIGN_MASK)
		return false;

	diff = { bsy1 - tFar };
	if (diff.i & SIGN_MASK)
		tFar = bsy1;

	diff = { tFar - bs[zs + 2] };
    if (diff.i & SIGN_MASK)
        return false;
    
	tNear = std::max(bs[zs + 2], tNear) - Constants::epsilon;
	tFar = std::min(bs[6 - zs], tFar) + Constants::epsilon;

	diff = { tFar - tNear };
    return !(diff.i & SIGN_MASK);
}

}
}
