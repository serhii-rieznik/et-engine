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

#define ET_RT_USE_RUSSIAN_ROULETTE 1

/*
 * PathTraceIntegrator
 */
float4 PathTraceIntegrator::gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength,
	KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
	/* TODO : to be removed, use evaluate instead

	if (maxPathLength == 0)
		maxPathLength = 0x7fffffff;

	float4 result(0.0f);
    Ray currentRay = inRay;

	float4 throughput(1.0f);
	for (pathLength = 0; pathLength < maxPathLength; ++pathLength)
	{
		KDTree::TraverseResult traverse = tree.traverse(currentRay);
		if (traverse.triangleIndex == InvalidIndex)
		{
			result += throughput * env->sampleInDirection(currentRay.direction);
			break;
		}

		const rt::Triangle& tri = tree.triangleAtIndex(traverse.triangleIndex);

		const rt::Material& mat = materials[tri.materialIndex];
		result += throughput * mat.emissive;

		float4 nrm = tri.interpolatedNormal(traverse.intersectionPointBarycentric);
		float4 uv0 = tri.interpolatedTexCoord0(traverse.intersectionPointBarycentric);

		BSDFSample sample(currentRay.direction, nrm, mat, uv0, et::rt::BSDFSample::Direction::Backward);
		throughput *= sample.combinedEvaluate();

		ET_ALIGNED(16) float local[4] = { };
		throughput.loadToFloats(local);
		float maxComponent = std::max(local[0], std::max(local[1], local[2]));

#	if (ET_RT_USE_RUSSIAN_ROULETTE)
		if (pathLength > 16)
		{
			float q = std::min(maxComponent, 0.95f);
			if (rt::fastRandomFloat() >= q)
				break;
			throughput /= q;
		}
#	endif

		currentRay.origin = traverse.intersectionPoint + sample.Wo * Constants::epsilon;
		currentRay.direction = sample.Wo;
	}

	static uint32_t aMax = 0;
	if (pathLength > aMax)
	{
		aMax = std::max(aMax, pathLength);
		log::info("Max path length: %llu", static_cast<uint64_t>(aMax));
	}
	*/

	return float4(0.0f);
}

/*
 * Normals
 */
float4 NormalsIntegrator::gather(const Ray& inRay, uint32_t maxPathLength,
	uint32_t& pathLenght, KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	return tri.interpolatedNormal(hit0.intersectionPointBarycentric) * 0.5f + float4(0.5f);
}

/*
 * Fresnel
 */
float4 FresnelIntegrator::gather(const Ray& inRay, uint32_t maxPathLength,
	uint32_t& pathLength, KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(inRay.direction);

	++pathLength;
	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	const auto& mat = materials[tri.materialIndex];
    float4 nrm = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
	float4 uv0 = tri.interpolatedTexCoord0(hit0.intersectionPointBarycentric);
	BSDFSample sample(inRay.direction, nrm, mat, uv0, BSDFSample::Direction::Backward);
	return float4(sample.fresnel);
}

// ao
float4 AmbientOcclusionIntegrator::gather(const Ray& inRay, uint32_t maxPathLength,
	uint32_t& pathLength, KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);

	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);
    float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;

	if (tree.traverse(Ray(nextOrigin, nextDirection)).triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(nextDirection);

	++pathLength;
	return float4(0.0f);
}

// hack-ao
float4 AmbientOcclusionHackIntegrator::gather(const Ray& inRay, uint32_t maxPathLength,
	uint32_t& pathLength, KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
	KDTree::TraverseResult hit0 = tree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(inRay.direction);

	const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
	float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);

	float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);
	float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;

	KDTree::TraverseResult t = tree.traverse(Ray(nextOrigin, nextDirection));
	if (t.triangleIndex == InvalidIndex)
		return float4(1.0f);

	++pathLength;

	float_type distance = (t.intersectionPoint - nextOrigin).length();
	return float4(1.0f - std::exp(-SQRT_2 * distance));
}

float4 BackwardPathTracingIntegrator::evaluate(const Scene& scene, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength)
{
	if (maxPathLength == 0)
		maxPathLength = 0x7FFFFFFF;

	const uint32_t lightSamples = 1;

	float4 result(0.0f);
	float4 throughput(1.0f);

    Ray currentRay = inRay;
	for (pathLength = 0; pathLength < maxPathLength; ++pathLength)
	{
		KDTree::TraverseResult hit = scene.kdTree.traverse(currentRay);
		if (hit.triangleIndex == InvalidIndex)
			break;

		const Triangle& tri = scene.kdTree.triangleAtIndex(hit.triangleIndex);
		const Material& mtl = scene.materials[tri.materialIndex];
		float4 nrm = tri.interpolatedNormal(hit.intersectionPointBarycentric);
		float4 nextPosition = hit.intersectionPoint + nrm * Constants::epsilon;
		float4 uv0 = tri.interpolatedTexCoord0(hit.intersectionPointBarycentric);

		BSDFSample sample(currentRay.direction, nrm, mtl, uv0, et::rt::BSDFSample::Direction::Backward);
		throughput *= sample.combinedEvaluate();

		if (lightSamples > 0)
		{
			float4 lightSamplesContrib = float4(0.0f);
			for (uint32_t ls = 0; ls < lightSamples; ++ls)
			{
				for (const Emitter::Pointer emitter : scene.emitters)
				{
					if (emitter->materialIndex() != tri.materialIndex)
					{
						EmitterInteraction i = emitter->sample(scene, nextPosition, nrm);
						float lightBsdf = std::max(0.0f, -i.normal.dot(i.direction));
						float surfaceBsdf = std::max(0.0f, nrm.dot(i.direction)) / PI;
						lightSamplesContrib += throughput * i.sample * (lightBsdf * surfaceBsdf);
					}
				}
			}
			result += lightSamplesContrib / static_cast<float>(lightSamples);
		}
		
		result += throughput * mtl.emissive;

#	if (ET_RT_USE_RUSSIAN_ROULETTE)
		if (pathLength > 16)
		{
			ET_ALIGNED(16) float local[4] = {};
			throughput.loadToFloats(local);
			float maxComponent = std::max(local[0], std::max(local[1], local[2]));
			float q = std::min(maxComponent, 0.95f);
			if (rt::fastRandomFloat() >= q)
				break;
			throughput /= q;
		}
#	endif

		currentRay.origin = nextPosition;
		currentRay.direction = sample.Wo;
	}

	return result;
}

}
}
