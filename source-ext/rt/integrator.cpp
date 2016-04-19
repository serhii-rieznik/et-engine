/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/integrator.h>

namespace et
{
namespace rt
{

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
    auto currentRay = inRay;

	maxDepth = 0;
    FastStack<MaxTraverseDepth, Bounce> bounces;
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

        float4 color;
        float brdf = 0.0f;

        currentRay.direction = reflectance(currentRay.direction, nrm, mat, color, brdf);
		currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;

		brdf *= nrm.dot(currentRay.direction);

#   if (ET_RT_VISUALIZE_BRDF)
		++maxDepth;
		return (brdf > 1.0f) ? float4(brdf - 1.0f, 0.0f, 0.0f, 1.0f) : float4(brdf, brdf, brdf, 1.0f);
#	else
        bounce.add = mat.emissive;
		bounce.scale = color * brdf;
#	endif
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

float4 PathTraceIntegrator::reflectance(const float4& incidence, float4& normal, const Material& mat,
    float4& color, float_type& brdf)
{
	switch (mat.type)
	{
		case MaterialType::Diffuse:
		{
            color = mat.diffuse;
            
            auto out = computeDiffuseVector(normal);
            brdf = lambert(normal, incidence, out, mat.roughness);
			return out;
		}

		case MaterialType::Conductor:
		{
            color = mat.specular;
            auto out = computeReflectionVector(incidence, normal, mat.roughness);
			auto fresnel = computeFresnelTerm<MaterialType::Conductor>(1.0f, -normal.dot(incidence));
			brdf = reflectionMicrofacet(normal, incidence, out, mat.roughness, fresnel);
			return out;
		}

		case MaterialType::Dielectric:
		{
            float eta = mat.ior;
            float IdotN = incidence.dot(normal);

			if (eta > 1.0f) // refractive
			{
				eta = 1.0f / eta;
				
				if (IdotN <= 0.0f) // entering material
				{
					IdotN = -IdotN;
				}
				else
				{
					normal *= -1.0f;
				}

				float_type k = computeRefractiveCoefficient(eta, IdotN);
				auto fresnel = computeFresnelTerm<MaterialType::Dielectric>(eta, IdotN);
				if ((k >= Constants::epsilon) && (fastRandomFloat() >= fresnel))
				{
					color = mat.diffuse;

					auto out = computeRefractionVector(incidence, normal, k, eta, IdotN, mat.roughness);
					brdf = -refractionMicrofacet(normal, incidence, reflect(out, normal), mat.roughness, fresnel);
					return out;
				}
				else
				{
					color = mat.specular;

					auto out = computeReflectionVector(incidence, normal, mat.roughness);
					brdf = reflectionMicrofacet(normal, incidence, out, mat.roughness, fresnel);
					return out;
				}
			}
			else // non-refractive material
			{
				auto fresnel = computeFresnelTerm<MaterialType::Dielectric>(eta, -IdotN);
				if (fastRandomFloat() > fresnel)
				{
                    color = mat.diffuse;
                    
                    auto out = computeDiffuseVector(normal);
                    brdf = lambert(normal, incidence, out, mat.roughness);
					return out;
				}
				else
				{
					color = mat.specular;
                    
                    auto out = computeReflectionVector(incidence, normal, mat.roughness);
                    brdf = reflectionMicrofacet(normal, incidence, out, mat.roughness, fresnel);
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
    float IdotN = inRay.direction.dot(normal);

    float value = 0.0f;
    switch (mat.type)
    {
        case MaterialType::Conductor:
        {
            value = computeFresnelTerm<MaterialType::Conductor>(0.0f, IdotN);
            break;
        }
            
        case MaterialType::Dielectric:
        {
            float_type eta = mat.ior > 1.0f ? 1.0f / mat.ior : mat.ior;
            value = computeFresnelTerm<MaterialType::Dielectric>(eta, IdotN);
            break;
        }
            
        default:
            ET_FAIL("Invalid material type");
    }
    
    return float4(value);
}

// ao
float4 AmbientOcclusionIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
	KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);

	float4 nextOrigin = hit0.intersectionPoint + surfaceNormal * Constants::epsilon;
	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);

	if (tree.traverse(Ray(nextOrigin, nextDirection)).triangleIndex == InvalidIndex)
	{
		++maxDepth;
		return env->sampleInDirection(nextDirection);
	}

	return float4(0.0f);
}


}
}