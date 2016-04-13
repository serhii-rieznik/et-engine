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
		return env->sampleInDirection(nextDirection);// * nextDirection.dot(surfaceNormal);
	}

    return float4(0.0f);
}
    
/*
 * PathTraceIntegrator
 */
struct ET_ALIGNED(16) Bounce
{
	float4 color;
	float4 emissive;

	Bounce() = default;
	Bounce(const float4& c, const float4& e) : color(c), emissive(e) { }
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
			bounces.emplace(float4(0.0f), env->sampleInDirection(currentRay.direction));
            break;
        }

		const auto& tri = tree.triangleAtIndex(traverse.triangleIndex);
		const auto& mat = materials[tri.materialIndex];
		const auto& normal = tri.interpolatedNormal(traverse.intersectionPointBarycentric);

		float4 color;
		currentRay.direction = selectNextDirection(currentRay.direction, normal, mat, color);
		currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;

		bounces.emplace(color, mat.emissive);
    }
	maxDepth = bounces.size();

    float4 result(0.0f);
    do
    {
		result = result * bounces.top().color;
		result += bounces.top().emissive;
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

float4 PathTraceIntegrator::selectNextDirection(const float4& Wi, float4 N, const Material& mat, float4& color)
{
	switch (mat.type)
	{
		case MaterialType::Diffuse:
		{
			auto Wo = randomVectorOnHemisphere(N, HALF_PI);
			color = mat.diffuse;
			return Wo;
		}

		case MaterialType::Conductor:
		{
			float4 idealReflection;
			auto Wo = computeReflectionVector(Wi, N, idealReflection, mat.distributionAngle);
			color = mat.specular;
			return Wo;
		}

		case MaterialType::Dielectric:
		{
			if (mat.ior > 1.0f) // refractive
			{
				float_type eta;
				if (N.dot(Wi) >= 0.0)
				{
					N *= -1.0f;
					eta = mat.ior;
				}
				else
				{
					eta = 1.00001f / mat.ior;
				}

				float_type k = computeRefractiveCoefficient(Wi, N, eta);
				auto fresnel = computeFresnelTerm(Wi, N, eta);
				if ((k >= Constants::epsilon) && (fastRandomFloat() >= fresnel))
				{
					color = mat.diffuse;
					float4 idealRefraction;
					return computeRefractionVector(Wi, N, k, eta, idealRefraction, mat.distributionAngle);
				}
				else
				{
					color = mat.specular;
					float4 idealReflection;
					return computeReflectionVector(Wi, N, idealReflection, mat.distributionAngle);
				}
			}
			else // non-refractive material
			{
				auto fresnel = computeFresnelTerm(Wi, N, mat.ior);
				if (fastRandomFloat() > fresnel)
				{
					color = mat.diffuse;
					return randomVectorOnHemisphere(N, HALF_PI);
				}
				else
				{
					color = mat.specular;
					float4 idealReflection;
					return computeReflectionVector(Wi, N, idealReflection, mat.distributionAngle);
				}
			}
			break;
		}
		default:
			ET_FAIL("Invalid material type");
	}

	return float4(1000.0f, 0.0f, 1000.0f, 1.0f);
}

float_type PathTraceIntegrator::brdf(const float4& Wi, const float4& Wo, const float4& N, const Material& mat)
{
	return 0.0f;
/*
	auto OdotN = Wo.dot(N);
	ET_ASSERT(OdotN >= 0.0f);

	if (mat.type == MaterialType::Diffuse)
	{
		return OdotN / PI;
	}

	auto R = rt::reflect(Wi, N);
	auto OdotR = std::max(0.0f, Wo.dot(R));
	auto sh = 2.0f / (mat.roughness * mat.roughness + Constants::epsilon) - 2.0f;
	auto val = std::pow(OdotR, sh);// * OdotN * (sh + 1.0f) / DOUBLE_PI;
	ET_ASSERT(!isnan(val));
	ET_ASSERT(!isinf(val));
	return val;
*/
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
	normal = randomVectorOnHemisphere(normal, mat.distributionAngle);
	
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