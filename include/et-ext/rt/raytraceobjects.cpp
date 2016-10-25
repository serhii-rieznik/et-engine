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

const float_type Constants::epsilon = 0.001f;
const float_type Constants::minusEpsilon = -epsilon;
const float_type Constants::onePlusEpsilon = 1.0f + epsilon;
const float_type Constants::oneMinusEpsilon = 1.0f - epsilon;
const float_type Constants::epsilonSquared = epsilon * epsilon;
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
    auto idealReflection = reflect(incidence, normal);
	
	auto result = randomVectorOnHemisphere(idealReflection, ggxDistribution, roughness);

	uint32_t attempts = 0;
	while ((result.dot(normal) <= 0.0f) && (attempts < 16))
	{
		result = randomVectorOnHemisphere(idealReflection, ggxDistribution, roughness);
		++attempts;
	}

	return result;
#endif
}

float4 computeRefractionVector(const float4& Wi, const float4& n, float_type eta, float roughness,
    float cosThetaI, float cosThetaT)
{
    auto idealRefraction = Wi * eta - n * (cosThetaI * eta - cosThetaT);
	auto result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);

	uint32_t attempts = 0;
	while ((result.dot(n) >= 0.0f) && (attempts < 16))
	{
		result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);
		++attempts;
	}

	return result;
}

bool rayToBoundingBox(const Ray& r, const BoundingBox& box, float& tNear, float& tFar)
{
	ET_ALIGNED(16) float_type localDir[4];
	ET_ALIGNED(16) float_type bounds[2][4];

	r.direction.loadToFloats(localDir);
	((box.minVertex() - r.origin) / r.direction).loadToFloats(bounds[0]);
	((box.maxVertex() - r.origin) / r.direction).loadToFloats(bounds[1]);

	index xNegative = floatIsNegative(localDir[0]);
	index yNegative = floatIsNegative(localDir[1]);
	float_type tmin = bounds[xNegative][0];
	float_type tmax = bounds[1 - xNegative][0];

	float_type tymin = bounds[yNegative][1];
	float_type tymax = bounds[1 - yNegative][1];

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    
    if (tymin > tmin)
        tmin = tymin;
    
    if (tymax < tmax)
        tmax = tymax;
    
	index zNegative = floatIsNegative(localDir[2]);
	float_type tzmin = bounds[zNegative][2];
	float_type tzmax = bounds[1 - zNegative][2];
    
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    
    if (tzmin > tmin)
        tmin = tzmin;
    
    if (tzmax < tmax)
        tmax = tzmax;
    
    tNear = tmin;
    tFar = tmax;
    
    return tmin <= tmax;
}

}
}
