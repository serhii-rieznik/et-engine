/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/integrator.h>

#define VISUALIZE_BRDF 0

namespace et
{
namespace rt
{
   
inline float lambert(const float4& n, const float4& Wo, float r)
{
    return 1.0f / PI;
}

inline float phong(const float4& n, const float4& Wi, const float4& Wo, const float4& r, float roughness)
{
    float Ns = std::max(0.0f, 2.0f / (sqr(roughness) + Constants::epsilon) - 2.0f);
    auto RdotW = clamp(r.dot(Wo), 0.0f, 1.0f);
    return clamp(std::pow(RdotW, Ns) * (Ns + 2.0f) / DOUBLE_PI, 0.0f, 1.0f);
}
    
float4 AmbientOcclusionIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
    KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
    KDTree::TraverseResult hit0 = tree.traverse(inRay);
    if (hit0.triangleIndex == InvalidIndex)
        return env->sampleInDirection(inRay.direction);
    
    const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
    float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, HALF_PI);
	float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;
	if (tree.traverse(Ray(nextOrigin, nextDirection)).triangleIndex == InvalidIndex)
	{
		++maxDepth;
		return env->sampleInDirection(nextDirection);// * nextDirection.dot(surfaceNormal);
	}

    return float4(0.0f);
}
    
/*
 * PathTraceIntegrator
 */
struct ET_ALIGNED(16) Bounce
{
    float4 scale;
    float4 add;
};
    
float4 PathTraceIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
    KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
    ++maxDepth;
    float4 l(0.0f, 1.0f, 0.0f, 0.0f);
    
    auto currentRay = inRay;
    
    FastStack<PathTraceIntegrator::MaxTraverseDepth, Bounce> bounces;
    while (bounces.size() < MaxTraverseDepth)
    {
        auto& bounce = bounces.emplace_back();
        KDTree::TraverseResult traverse = tree.traverse(currentRay);
        if (traverse.triangleIndex == InvalidIndex)
        {
            bounce.add = env->sampleInDirection(currentRay.direction);
            break;
        }
        
		const auto& tri = tree.triangleAtIndex(traverse.triangleIndex);
        const auto& mat = materials[tri.materialIndex];
        auto nrm = tri.interpolatedNormal(traverse.intersectionPointBarycentric);

#   if (VISUALIZE_BRDF)
        auto r = reflect(currentRay.direction, nrm);
        return float4(phong(nrm, currentRay.direction, l, r, mat.roughnessValue));
#   else
        float4 color;
        float brdf = 0.0f;
        currentRay.direction = reflectance(currentRay.direction, nrm, mat, color, brdf);
		currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;
        bounce.add = mat.emissive;
        bounce.scale = color * brdf;
#   endif
        
    }
	maxDepth = bounces.size();
    
    float4 result(0.0f);
    do
    {
        result *= bounces.top().scale;
        result += bounces.top().add;
        bounces.pop();
    }
    while (bounces.hasSomething());
    
    return result;
}

inline float4 computeReflectionVector(const float4& incidence, const float4& normal, float4& idealReflection, float distribution)
{
	idealReflection = reflect(incidence, normal);
    
	auto direction = randomVectorOnHemisphere(idealReflection, distribution);
	if (direction.dot(normal) < 0.0f)
		direction = reflect(direction, normal);
	return direction;
}

inline float4 computeRefractionVector(const float4& incidence, const float4& normal,
	float_type k, float_type eta, float4& idealRefraction, float distribution)
{
    idealRefraction = incidence * eta - normal * (eta * normal.dot(incidence) + std::sqrt(k));
    
	auto direction = randomVectorOnHemisphere(idealRefraction, distribution);
    if (direction.dot(normal) > 0.0f)
        direction = reflect(direction, normal);
	return direction;
}

float4 PathTraceIntegrator::reflectance(const float4& incidence, float4& normal, const Material& mat,
    float4& color, float_type& brdf)
{
    brdf = 1.0f;
    
	switch (mat.type)
	{
		case MaterialType::Diffuse:
		{
            color = mat.diffuse;
            
            auto out = randomVectorOnHemisphere(normal, HALF_PI);
            brdf = lambert(normal, out, mat.roughnessValue);
			return out;
		}

		case MaterialType::Conductor:
		{
            color = mat.specular;
            float4 idealReflection;
            auto out = computeReflectionVector(incidence, normal, idealReflection, mat.distributionAngle);
            brdf = phong(normal, incidence, out, idealReflection, mat.roughnessValue);
			return out;
		}

		case MaterialType::Dielectric:
		{
			if (mat.ior > 1.0f) // refractive
			{
				float_type eta;
				if (normal.dot(incidence) >= 0.0)
				{
					normal *= -1.0f;
					eta = mat.ior;
				}
				else
				{
					eta = 1.00001f / mat.ior;
				}

				float_type k = computeRefractiveCoefficient(incidence, normal, eta);
                auto fresnel = computeFresnelTerm<MaterialType::Dielectric>(incidence, normal, eta);
				if ((k >= Constants::epsilon) && (fastRandomFloat() >= fresnel))
				{
					color = mat.diffuse;
                    
                    float4 idealRefraction;
                    auto out = computeRefractionVector(incidence, normal, k, eta, idealRefraction, mat.distributionAngle);
                    brdf = phong(normal, incidence, out, idealRefraction, mat.roughnessValue);
                    
                    normal *= -1.0f;
					return out;
				}
				else
				{
					color = mat.specular;
                    
                    float4 idealReflection;
                    auto out = computeReflectionVector(incidence, normal, idealReflection, mat.distributionAngle);
                    brdf = phong(normal, incidence, out, idealReflection, mat.roughnessValue);
					return out;
				}
			}
			else // non-refractive material
			{
				auto fresnel = computeFresnelTerm<MaterialType::Dielectric>(incidence, normal, mat.ior);
				if (fastRandomFloat() > fresnel)
				{
                    color = mat.diffuse;
                    
                    auto out = randomVectorOnHemisphere(normal, HALF_PI);
                    brdf = lambert(normal, out, mat.roughnessValue);
					return out;
				}
				else
				{
					color = mat.specular;
                    
                    float4 idealReflection;
                    auto out = computeReflectionVector(incidence, normal, idealReflection, mat.distributionAngle);
                    brdf = phong(normal, incidence, out, idealReflection, mat.roughnessValue);
					return out;
				}
			}
			break;
		}
		default:
			ET_FAIL("Invalid material type");
	}

	return float4(1000.0f, 0.0f, 1000.0f, 1.0f);
}

/*
 * Normals
 */
float4 NormalsIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth, KDTree& tree,
	EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	return tri.interpolatedNormal(hit0.intersectionPointBarycentric) * 0.5f + float4(0.5f);
}

/*
 * Fresnel
 */
float4 FresnelIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth, KDTree& tree,
	EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return env->sampleInDirection(inRay.direction);

	++maxDepth;
	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	const auto& mat = materials[tri.materialIndex];
    float4 normal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);

    float value = 0.0f;
    switch (mat.type)
    {
        case MaterialType::Conductor:
        {
            value = computeFresnelTerm<MaterialType::Conductor>(inRay.direction, normal, 0.0f);
            break;
        }
            
        case MaterialType::Dielectric:
        {
            float_type eta = mat.ior > 1.0f ? 1.0f / mat.ior : mat.ior;
            value = computeFresnelTerm<MaterialType::Dielectric>(inRay.direction, normal, eta);
            break;
        }
            
        default:
            ET_FAIL("Invalid material type");
    }
    
    return float4(value);
}

}
}