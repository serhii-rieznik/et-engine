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

/*
float reflectionMicrofacet(const float4& n, const float4& Wi, const float4& Wo, float r, float f)
{
	auto h = Wo - Wi + n * Constants::epsilonSquared;
	h.normalize();

	float NdotO = n.dot(Wo);
	float NdotI = -n.dot(Wi);
	float HdotO = h.dot(Wo);
	float HdotI = -h.dot(Wi);
	float NdotH = n.dot(h);
	float rSq = r * r + Constants::epsilon;

	float g1 = G_ggx(NdotI, rSq) * float(HdotI / NdotI > 0.0f);
	float g2 = G_ggx(NdotO, rSq) * float(HdotO / NdotO > 0.0f);
	float g = g1 * g2;
	ET_ASSERT(!isnan(g));

	// float d = D_ggx(rSq, NdotH) * float(NdotH > 0.0f);
	// float brdf = (d * g * f) / (4.0f * NdotI * NdotO);
	// float pdf = d * NdotH / (4.0f * HdotI);

	float result = (g * f * HdotI) / (NdotI * NdotH + Constants::epsilon); // x NdotO
	ET_ASSERT(!isnan(result));

	return result;
}

float refractionMicrofacet(const float4& n, const float4& Wi, const float4& Wo, float r, float f, float eta)
{

}
*/

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
	for (size_t attempt = 0; (attempt < 16) && (result.dot(normal) <= 0.0f); ++attempt)
	{
		result = randomVectorOnHemisphere(idealReflection, ggxDistribution, roughness);
	}
	return result;
#endif
}

float4 computeRefractionVector(const float4& Wi, const float4& n, float_type eta, float roughness,
    float cosThetaI, float cosThetaT)
{
    auto idealRefraction = Wi * eta - n * (cosThetaI * eta - cosThetaT);
	auto result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);
	for (size_t attempt = 0; (attempt < 16) && (result.dot(n) >= 0.0f); ++attempt)
	{
		result = randomVectorOnHemisphere(idealRefraction, ggxDistribution, roughness);
	}
	return result;
}

bool rayToBoundingBox(const Ray& r, const BoundingBox& box, float& tNear, float& tFar)
{
    float4 bounds[2] = { box.minVertex(), box.maxVertex() };
    
    float_type tmin, tmax, tymin, tymax, tzmin, tzmax;
    
    if (r.direction.cX() >= 0)
    {
        tmin = (bounds[0].cX() - r.origin.cX()) / r.direction.cX() - Constants::epsilon;
        tmax = (bounds[1].cX() - r.origin.cX()) / r.direction.cX() + Constants::epsilon;
    }
    else
    {
        tmin = (bounds[1].cX() - r.origin.cX()) / r.direction.cX() - Constants::epsilon;
        tmax = (bounds[0].cX() - r.origin.cX()) / r.direction.cX() + Constants::epsilon;
    }
    
    if (r.direction.cY() >= 0)
    {
        tymin = (bounds[0].cY() - r.origin.cY()) / r.direction.cY() - Constants::epsilon;
        tymax = (bounds[1].cY() - r.origin.cY()) / r.direction.cY() + Constants::epsilon;
    }
    else
    {
        tymin = (bounds[1].cY() - r.origin.cY()) / r.direction.cY() - Constants::epsilon;
        tymax = (bounds[0].cY() - r.origin.cY()) / r.direction.cY() + Constants::epsilon;
    }
    
    if ((tmin > tymax) || (tymin > tmax))
        return false;
    
    if (tymin > tmin)
        tmin = tymin;
    
    if (tymax < tmax)
        tmax = tymax;
    
    if (r.direction.cZ() >= 0)
    {
        tzmin = (bounds[0].cZ() - r.origin.cZ()) / r.direction.cZ() - Constants::epsilon;
        tzmax = (bounds[1].cZ() - r.origin.cZ()) / r.direction.cZ() + Constants::epsilon;
    }
    else
    {
        tzmin = (bounds[1].cZ() - r.origin.cZ()) / r.direction.cZ() - Constants::epsilon;
        tzmax = (bounds[0].cZ() - r.origin.cZ()) / r.direction.cZ() + Constants::epsilon;
    }
    
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
