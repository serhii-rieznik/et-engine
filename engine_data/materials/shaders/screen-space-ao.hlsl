#include <et>
#include <inputdefines>
#include <options>

Texture2D<float> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

Texture2D<float4> noiseTexture : DECL_TEXTURE(Noise);
SamplerState noiseSampler : DECL_SAMPLER(Noise);

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
    row_major float4x4 viewTransform;
    row_major float4x4 projectionTransform;
    row_major float4x4 viewProjectionTransform;
    row_major float4x4 inverseViewProjectionTransform;
    row_major float4x4 inverseProjectionTransform;
    float2 cameraJitter;
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
                    
float3 reconstructNormal(in float2 uv, in float2 texelSize)
{
	float2 uv0 = uv - float2(texelSize.x, 0.0);
	float2 uv1 = uv + float2(texelSize.x, 0.0);
	float2 uv2 = uv - float2(0.0, texelSize.y);
	float2 uv3 = uv + float2(0.0, texelSize.y);
	
	float z = baseColorTexture.Sample(baseColorSampler, uv);
	float z0 = baseColorTexture.Sample(baseColorSampler, uv0);
	float z1 = baseColorTexture.Sample(baseColorSampler, uv1);	
	float z2 = baseColorTexture.Sample(baseColorSampler, uv2);
	float z3 = baseColorTexture.Sample(baseColorSampler, uv3);
	float3 pC = viewSpace(uv * 2.0 - 1.0, z);
	float3 p0 = viewSpace(uv0 * 2.0 - 1.0, z0);
	float3 p1 = viewSpace(uv1 * 2.0 - 1.0, z1);
	float3 p2 = viewSpace(uv2 * 2.0 - 1.0, z2);
	float3 p3 = viewSpace(uv3 * 2.0 - 1.0, z3);

	return normalize(cross(p3 - p2, p1 - p0));
}

const uint directionsCount = 8;

float3 getRotatedDirection(in uint index, in float cs)
{
	const float3 predefinedDirection[] = {
        float3(1.000000, 0.000000, 0.0),
        float3(0.707107, 0.707107, 0.0),
        float3(-0.000000, 1.000000, 0.0),
        float3(-0.707107, 0.707107, 0.0),
        float3(-1.000000, -0.000000, 0.0),
        float3(-0.707107, -0.707107, 0.0),
        float3(0.000000, -1.000000, 0.0),
        float3(0.707107, -0.707107, 0.0),
	};
	float2 base = predefinedDirection[index];

	cs = cs * 2.0 - 1.0;
	float sn = sqrt(1.0 - cs * cs); 
	return float3(dot(base, float2(cs, -sn)), dot(base, float2(sn, cs)), 0.0);
}

float4 sampleAmbientOcclusion(in float2 uv, in float2 texelSize, in float4 rnd)
{
	const uint samplesPerDirection = 8;
	const float influenceRadius = 1.0 * (rnd.y * 0.5 + 0.5);
	const float deltaTheta = 2.0 * PI / float(directionsCount);

	float3 n0 = reconstructNormal(uv, texelSize);
	float3 p0 = viewSpace(uv * 2.0 - 1.0, baseColorTexture.Sample(baseColorSampler, uv) - 0.000005);

	float ao = 0.0;
	float theta = (rnd.x * 2.0 - 1.0) * PI;
	for (uint i = 0; i < directionsCount; ++i)
	{
		float3 direction = getRotatedDirection(i, rnd.z);
		float3 pEndProjected = projectionSpace(p0 + direction * influenceRadius) * 0.5 + 0.5;
		float2 projectedStep = (pEndProjected.xy - uv) / float(samplesPerDirection);

		float2 texelStepUv = uv + direction.xy * texelSize;
		float directionZ = baseColorTexture.Sample(baseColorSampler, texelStepUv);
		float3 tangentVector = (viewSpace(texelStepUv * 2.0 - 1.0, directionZ) - p0);
		tangentVector -= dot(tangentVector, n0) * n0;

		float maxOcclusion = 0.0;
		for (uint j = 1; j <= samplesPerDirection; ++j)
		{
			float2 stepUv = uv + projectedStep * float(j);
			float stepZ = baseColorTexture.Sample(baseColorSampler, stepUv);
			float3 stepViewSpace = viewSpace(stepUv * 2.0 - 1.0, stepZ);

			float3 horizonVector = stepViewSpace - p0;
			float horizonVectorLength = length(horizonVector);

			float occlusion = dot(n0, horizonVector / horizonVectorLength);
			float occlusionDifference = max(0.0, occlusion - maxOcclusion);
			maxOcclusion = max(maxOcclusion, occlusion);
			float attenuation = 1.0 - saturate(horizonVectorLength / influenceRadius);
			ao += occlusionDifference * attenuation;
		}

		theta += deltaTheta;
	}

	ao = 1.0 - (ao * deltaTheta) / (2.0 * PI);
	return ao * ao;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	baseColorTexture.GetDimensions(0, w, h, levels);
	float2 texelSize = 1.0 / float2(w, h);

	noiseTexture.GetDimensions(0, w, h, levels);
	float2 noiseTexelSize = float2(w, h);

	float2 noiseUv = fsIn.texCoord0 * (viewport.zw / noiseTexelSize + continuousTime);
	float4 sampledNoise = noiseTexture.SampleLevel(noiseSampler, noiseUv, 0.0);

	return sampleAmbientOcclusion(fsIn.texCoord0, texelSize, sampledNoise);
}
                                                                