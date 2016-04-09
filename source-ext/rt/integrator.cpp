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
		return env->sampleInDirection(nextDirection) * nextDirection.dot(surfaceNormal);
	}

    return float4(0.0f);
}
    
/*
 * PathTraceIntegrator
 */
struct ET_ALIGNED(16) Bounce
{
    float4 add;
    float4 mul;
    Bounce() = default;
    Bounce(const float4& a, const float4& m) :
        add(a), mul(m) { }
};
    
float4 PathTraceIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
    KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
    auto currentRay = inRay;

    FastStack<PathTraceIntegrator::MaxTraverseDepth, Bounce> bounces;
    while (bounces.size() < MaxTraverseDepth)
    {
        KDTree::TraverseResult traverse = tree.traverse(currentRay);
        if (traverse.triangleIndex == InvalidIndex)
        {
            bounces.emplace(env->sampleInDirection(currentRay.direction), float4(0.0f));
            break;
        }

        const auto& tri = tree.triangleAtIndex(traverse.triangleIndex);
		const auto& mat = materials[tri.materialIndex];
		auto incidence = currentRay.direction;
        float4 normal = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
		float4 outputColor = compute(normal, mat, incidence, currentRay.direction);
        currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;
		bounces.emplace(mat.emissive, outputColor);
    }
	maxDepth = bounces.size();

    float4 result(0.0f);
    do
    {
        result *= bounces.top().mul;
        result += bounces.top().add;
        bounces.pop();
    }
    while (bounces.hasSomething());


    return result;
}

inline float4 computeReflectionVector(const float4& incidence, const float4& normal, float4& idealReflection, float roughness)
{
	idealReflection = reflect(incidence, normal);
	auto direction = randomVectorOnHemisphere(idealReflection, roughness);
	if (direction.dot(normal) < 0.0f)
		direction = reflect(direction, normal);
	return direction;
}

inline float4 computeRefractionVector(const float4& incidence, const float4& normal,
	float_type k, float_type eta, float4& idealRefraction, float roughness)
{
    idealRefraction = incidence * eta - normal * (eta * normal.dot(incidence) + std::sqrt(k));
	auto direction = randomVectorOnHemisphere(idealRefraction, roughness);
    if (direction.dot(normal) >= Constants::minusEpsilon)
        direction = reflect(direction, normal);
	return direction;
}
    
float4 PathTraceIntegrator::compute(float4& normal, const Material& mat, const float4& incidence,
	float4& newDirection)
{
	switch (mat.type)
	{
		case MaterialType::Diffuse:
		{
			newDirection = randomVectorOnHemisphere(normal, HALF_PI);
			auto scale = newDirection.dot(normal);
			return mat.diffuse * scale;
		}

		case MaterialType::Conductor:
		{
			float4 idealReflection;
			newDirection = computeReflectionVector(incidence, normal, idealReflection, mat.roughness);
			return mat.specular * idealReflection.dot(newDirection);
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
				auto fresnel = computeFresnelTerm(incidence, normal, eta);
				if ((k >= Constants::epsilon) && (fastRandomFloat() >= fresnel))
				{
					float4 idealRefraction;
					newDirection = computeRefractionVector(incidence, normal, k, eta, idealRefraction, mat.roughness);
					return mat.diffuse * idealRefraction.dot(newDirection);
				}
				else
				{
					float4 idealReflection;
					newDirection = computeReflectionVector(incidence, normal, idealReflection, mat.roughness);
					return mat.specular * idealReflection.dot(newDirection);
				}
			}
			else // non-refractive material
			{
				auto fresnel = computeFresnelTerm(incidence, normal, mat.ior);
				if (fastRandomFloat() > fresnel)
				{
					newDirection = randomVectorOnHemisphere(normal, HALF_PI);
					return mat.diffuse * newDirection.dot(normal);
				}
				else
				{
					auto reflected = reflect(incidence, normal);
					newDirection = randomVectorOnHemisphere(reflected, mat.roughness);
					if (newDirection.dot(normal) < 0.0f)
						newDirection = reflect(newDirection, normal);
					auto scale = newDirection.dot(reflected);
					return mat.specular * scale;
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

	if (mat.ior == 0.0f)
		return float4(1.0f, 0.0f, 1.0f, 0.0f);

	float4 normal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
	normal = randomVectorOnHemisphere(normal, mat.roughness);
	
	float_type eta = mat.ior > 1.0f ? 1.0f / mat.ior : mat.ior;
	float_type k = computeRefractiveCoefficient(inRay.direction, normal, eta);
	if (k >= Constants::epsilon)
	{
		return float4(computeFresnelTerm(inRay.direction, normal, eta));
	}

	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

}
}