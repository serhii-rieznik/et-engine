#include <et>
#include <inputdefines>
#include <options>

Texture2D<float> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
    row_major float4x4 viewTransform;
    row_major float4x4 projectionTransform;
    row_major float4x4 viewProjectionTransform;
    row_major float4x4 inverseViewProjectionTransform;
    row_major float4x4 inverseProjectionTransform;
    float4 lightDirection;
    float4 viewport;
    float2 cameraClipPlanes;
    float continuousTime;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

#include "vertexprogram-2d-triangle.h"

float3 viewSpace(in float2 uv, in float z)
{
	float2 uvScale = float2(-inverseProjectionTransform[0][0], -inverseProjectionTransform[1][1]);
	float linearZ = cameraClipPlanes.x * cameraClipPlanes.y / (z * (cameraClipPlanes.y - cameraClipPlanes.x) - cameraClipPlanes.y);
	return float3(uv * uvScale, 1.0) * linearZ;
}

float3 projectionSpace(in float3 coord)
{
	float2 uvScale = float2(-inverseProjectionTransform[0][0], -inverseProjectionTransform[1][1]);
	float encodedZ = (cameraClipPlanes.x * cameraClipPlanes.y / coord.z + cameraClipPlanes.y) / (cameraClipPlanes.y - cameraClipPlanes.x);
	return float3(coord.xy / (coord.z * uvScale), encodedZ);
}

float rand(float n) 
{
	return frac(sin(n) * 43758.5453123);
}

float noise(float p)
{
	float fl = floor(p);
	float fc = frac(p);
	return lerp(rand(fl), rand(fl + 1.0), fc);
}

float4 sampleShadowsInWorldSpace(in float2 uv)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	baseColorTexture.GetDimensions(0, w, h, levels);

	float4 projectedLight = mul(lightDirection, viewTransform);
	projectedLight.xyz = normalize(projectedLight.xyz);

	float z0 = baseColorTexture.Sample(baseColorSampler, uv);
	float3 viewSpaceCoord = viewSpace(uv * 2.0 - 1.0, z0);
	float3 projectionSpaceCoord = projectionSpace(viewSpaceCoord);

	float3 p1 = projectionSpace(viewSpaceCoord + projectedLight);
	
	float3 lightStep = p1 - projectionSpaceCoord;
	float stepSize = 0.25 * length(lightStep.xy);
	float pixelStepSize = length(lightStep.xy / float2(stepSize * w, stepSize * h));
	lightStep *= pixelStepSize / stepSize;

	float lightContribution = 1.0;
	const uint maxSamples = 50;
	for (uint i = 0; i < maxSamples; ++i)
	{
		projectionSpaceCoord += lightStep;

		float sampledZ = baseColorTexture.Sample(baseColorSampler, projectionSpaceCoord.xy * 0.5 + 0.5);
		float zTest = step(projectionSpaceCoord.z, sampledZ);

		float3 projectedDir = normalize(viewSpace(projectionSpaceCoord.xy, sampledZ) - viewSpaceCoord);
		float ds = 1.0 - float(i) / float(maxSamples);
		float threshold = 0.9999 - 0.009 * ds * ds;
		float directionTest = step(threshold, dot(projectedDir, projectedLight));

		lightContribution *= 1.0 - zTest * directionTest;
	}

	return lightContribution;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return sampleShadowsInWorldSpace(fsIn.texCoord0);
}
