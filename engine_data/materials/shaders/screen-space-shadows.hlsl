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

float3 viewSpace(in float3 coord)
{
	float2 uvScale = float2(-inverseProjectionTransform[0][0], -inverseProjectionTransform[1][1]);
	float linearZ = cameraClipPlanes.x * cameraClipPlanes.y / (coord.z * (cameraClipPlanes.y - cameraClipPlanes.x) - cameraClipPlanes.y);
	return float3(coord.xy * uvScale, 1.0) * linearZ;
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
	float z0 = baseColorTexture.Sample(baseColorSampler, uv);
	
	if (z0 == 1.0)
		return 1.0;

	float4 projectedLight = mul(lightDirection, viewTransform);
	projectedLight.xyz = normalize(projectedLight.xyz);

	float3 screenSpace = float3(uv * 2.0 - 1.0, z0);
	float3 viewSpaceCoord = viewSpace(screenSpace);
	float3 projectionSpaceCoord = projectionSpace(viewSpaceCoord);

	float3 p1 = projectionSpace(viewSpaceCoord + projectedLight);
	float3 projectionSpaceLight = p1 - projectionSpaceCoord;
	
	float3 lightStep = projectionSpaceLight;

	float rnd = noise(continuousTime + (uv.y + lightStep.x) * 43758.5453123 + (uv.x + lightStep.y) * 100000.0);

	float lightContribution = 1.0;
	const uint maxSamples = 200;
	for (uint i = 0; i < maxSamples; ++i)
	{
		float distanceScale = float(i) / maxSamples;
		rnd = noise(rnd * 43758.5453123);
		projectionSpaceCoord += lightStep * (rnd * distanceScale);

		float sampledZ = baseColorTexture.Sample(baseColorSampler, projectionSpaceCoord.xy * 0.5 + 0.5);
		if (sampledZ < projectionSpaceCoord.z)
		{
			float3 projectedDir = normalize(
				viewSpace(float3(projectionSpaceCoord.xy, sampledZ)) - viewSpaceCoord
			);

			if (dot(projectedDir.xyz, projectedLight.xyz) > 0.999)
			{
				float uvScale = all(clamp(projectionSpaceCoord.xy, -1.0, 1.0) == projectionSpaceCoord.xy);
				lightContribution = 1.0 - uvScale;
				break;
			}
		}
	}

	return lightContribution;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return sampleShadowsInWorldSpace(fsIn.texCoord0);
}
