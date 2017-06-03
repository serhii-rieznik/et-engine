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

float4 evaluateNormals(const Scene& scene, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength)
{
	KDTree::TraverseResult hit0 = scene.kdTree.traverse(inRay);
	if (hit0.triangleIndex == InvalidIndex)
		return float4(1.0f); // TODO : sample light? env->sampleInDirection(inRay.direction);

	const auto& tri = scene.kdTree.triangleAtIndex(hit0.triangleIndex);
	return tri.interpolatedNormal(hit0.intersectionPointBarycentric) * 0.5f + float4(0.5f);
}

float4 evaluateAmbientOcclusion(const Scene& scene, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength)
{
	float4 result(1.0f);
	KDTree::TraverseResult hit0 = scene.kdTree.traverse(inRay);
	if (hit0.triangleIndex != InvalidIndex)
	{
		++pathLength;
		
		const Triangle& tri = scene.kdTree.triangleAtIndex(hit0.triangleIndex);
		float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
		float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, uniformDistribution);
		if (scene.kdTree.traverse(Ray(hit0.intersectionPoint, nextDirection)).triangleIndex != InvalidIndex)
			result = float4(0.0f);
	}
	return result;
}

float4 evaluateGlobalIllumination(const Scene& scene, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength)
{
	if (maxPathLength == 0)
		maxPathLength = 0x7FFFFFFF;

	float4 result(0.0f);
	float4 throughput(1.0f);

	Ray currentRay = inRay;
	for (pathLength = 0; pathLength < maxPathLength; ++pathLength)
	{
		KDTree::TraverseResult intersection = scene.kdTree.traverse(currentRay);
		if (intersection.triangleIndex == InvalidIndex)
		{
			for (const Emitter::Pointer& em : scene.emitters)
			{
				if (em->type() == Emitter::Type::Uniform)
				{
					float pdf;
					float4 nrm;
					float4 pos;
					float4 l = em->evaluate(scene, currentRay.origin, currentRay.direction, nrm, pos, pdf);
					result += throughput * l;
				}
			}
			break;
		}

		const Triangle& tri = scene.kdTree.triangleAtIndex(intersection.triangleIndex);
		const Material& mtl = scene.materials[tri.materialIndex];
		float4 nrm = tri.interpolatedNormal(intersection.intersectionPointBarycentric);
		float4 uv0 = tri.interpolatedTexCoord0(intersection.intersectionPointBarycentric);

		BSDFSample bsdfSample(currentRay.direction, nrm, mtl, uv0);

		result += throughput * mtl.emissive;
		throughput *= bsdfSample.evaluate();

		currentRay.origin = intersection.intersectionPoint;
		currentRay.direction = bsdfSample.Wo;

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

	}

	return result;
}

}
}
