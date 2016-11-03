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

#define ET_RT_NEW_STYLE 1

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
	maxDepth = 0;
	float4 result(0.0f);
    Ray currentRay = inRay;

#if (ET_RT_NEW_STYLE)

	const size_t maximumPathLength = 0x00ffffff;
	float4 throughput(1.0f);
	for (; maxDepth < maximumPathLength; ++maxDepth)
	{
		KDTree::TraverseResult traverse = tree.traverse(currentRay);
		if (traverse.triangleIndex == InvalidIndex)
		{
			result += throughput * env->sampleInDirection(currentRay.direction);
			break;
		}

		const rt::Triangle& tri = tree.triangleAtIndex(traverse.triangleIndex);
		const rt::Material& mat = materials[tri.materialIndex];
		float4 nrm = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
		float4 uv0 = tri.interpolatedTexCoord0(traverse.intersectionPointBarycentric);

		BSDFSample sample(currentRay.direction, nrm, mat, uv0, et::rt::BSDFSample::Direction::Backward);

		result += throughput * mat.emissive;
		throughput *= sample.combinedEvaluate();

		ET_ALIGNED(16) float local[4] = { };
		throughput.loadToFloats(local);
		float maxComponent = std::max(local[0], std::max(local[1], local[2]));
		//
		if (maxComponent < rt::Constants::epsilon)
			break;
		// */

		//
		if (maxDepth > 5)
		{
			float q = std::min(maxComponent, 0.95f);
			if (rt::fastRandomFloat() >= q)
				break;
			throughput /= q;
		}
		// */

		currentRay.origin = traverse.intersectionPoint + sample.Wo * Constants::epsilon;
		currentRay.direction = sample.Wo;
	}

	static size_t aMax = 0;
	static size_t aMin = 100000000;
	if ((maxDepth < aMin) || (maxDepth > aMax))
	{
		aMin = std::min(aMin, maxDepth);
		aMax = std::max(aMax, maxDepth);
		log::info("Min/Max path length: %llu / %llu", static_cast<uint64_t>(aMin), static_cast<uint64_t>(aMax));
	}

#else

	FastStack<MaxTraverseDepth, Bounce> bounces;
    while (bounces.size() < MaxTraverseDepth)
    {
        Bounce& bounce = bounces.emplace_back();

        KDTree::TraverseResult traverse = tree.traverse(currentRay);
        if (traverse.triangleIndex == InvalidIndex)
        {
            bounce.add = env->sampleInDirection(currentRay.direction);
            break;
        }
		++maxDepth;

		const auto& tri = tree.triangleAtIndex(traverse.triangleIndex);
		const auto& mat = materials[tri.materialIndex];
        float4 nrm = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
		float4 uv0 = tri.interpolatedTexCoord0(traverse.intersectionPointBarycentric);

		BSDFSample sample(currentRay.direction, nrm, mat, uv0, et::rt::BSDFSample::Direction::Backward);
		bounce.scale = sample.combinedEvaluate();
		bounce.add = mat.emissive;

#	if ET_RT_VISUALIZE_BRDF
		maxDepth = 5;
		return bounce.scale;
#	else
		currentRay.direction = sample.Wo;
		currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;
#	endif
    }

    do
    {
        result *= bounces.top().scale;
        result += bounces.top().add;
        bounces.pop();
    }
    while (bounces.hasSomething());
#endif

    return result;
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
    float4 nrm = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
	float4 uv0 = tri.interpolatedTexCoord0(hit0.intersectionPointBarycentric);
	BSDFSample sample(inRay.direction, nrm, mat, uv0, BSDFSample::Direction::Backward);
	return float4(sample.fresnel);
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

	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);
    float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;

	if (tree.traverse(Ray(nextOrigin, nextDirection)).triangleIndex == InvalidIndex)
		return env->sampleInDirection(nextDirection);

	++maxDepth;
	return float4(0.0f);
}

// hack-ao
float4 AmbientOcclusionHackIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
	KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);

	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);
	float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;

	KDTree::TraverseResult t = tree.traverse(Ray(nextOrigin, nextDirection));
	if (t.triangleIndex == InvalidIndex)
		return float4(1.0f);

	++maxDepth;

	float_type distance = (t.intersectionPoint - nextOrigin).length();
	return float4(1.0f - std::exp(-SQRT_2 * distance));
}

}
}
