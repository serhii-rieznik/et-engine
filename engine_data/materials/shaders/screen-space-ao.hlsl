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
    float4 cameraJitter;
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

static const float aoScale = 10.0;
static const uint directionsCount = 8;

float3 getRotatedDirection(in uint index, in float4 m)
{
	const float3 predefinedDirection[8] = {
        float3(1.0, 0.0, 0.0),
        float3(0.707107, 0.707107, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(-0.707107, 0.707107, 0.0),
        float3(-1.0, 0.0, 0.0),
        float3(-0.707107, -0.707107, 0.0),
        float3(0.0, -1.0, 0.0),
        float3(0.707107, -0.707107, 0.0),
	};
	float2 base = predefinedDirection[index];
	return float3(dot(base, m.xy), dot(base, m.zw), 0.0);
}

float4 sampleAmbientOcclusion(in float2 uv, in float2 texelSize, in float4 rnd)
{
	const uint samplesPerDirection = 6;
	const float influenceRadius = 0.5 * (rnd.x * 0.5 + 0.5); // 1.0 * (rnd.x * 0.5 + 0.5);

	float3 p0 = viewSpace(uv * 2.0 - 1.0, baseColorTexture.Sample(baseColorSampler, uv));
	float3 n0 = reconstructNormal(uv, texelSize);

	float directionStepSize = -influenceRadius / (p0.z * float(samplesPerDirection));

	float rotCos = rnd.z * 2.0 - 1.0;
	float rotSin = rnd.w * 2.0 - 1.0;
	
	float4 rotationMatrix;
	rotationMatrix.xy = float2(rotCos, -rotSin);
	rotationMatrix.zw = float2(rotSin,  rotCos);

	float ao = 0.0;
	for (uint i = 0; i < directionsCount; ++i)
	{
		float3 direction = getRotatedDirection(i, rotationMatrix);
		float2 projectedStep = direction * directionStepSize;
		float2 stepUv = uv + projectedStep;
		for (uint j = 0; j < samplesPerDirection; ++j)
		{
			float3 stepViewSpace = viewSpace(stepUv * 2.0 - 1.0, baseColorTexture.Sample(baseColorSampler, stepUv));
			float3 horizonVector = stepViewSpace - p0;
			float horizonVectorLength = length(horizonVector);
			float attenuation = saturate(1.0 - horizonVectorLength / influenceRadius);
			ao += attenuation * saturate(dot(n0, horizonVector) / horizonVectorLength);
			stepUv += projectedStep;
		}
	}
	return saturate(1.0 - aoScale * ao / float(directionsCount * samplesPerDirection));
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float w = viewport.z;
	float h = viewport.w;
	float levels = 0.0;
	baseColorTexture.GetDimensions(0, w, h, levels);
	float2 texelSize = 1.0 / float2(w, h);

	w = 64.0;
	h = 64.0;
	noiseTexture.GetDimensions(0, w, h, levels);
	float2 noiseTexelSize = float2(w, h);

	float2 noiseUv = frac(continuousTime + fsIn.texCoord0 * (viewport.zw / noiseTexelSize));
	
	float cs = cos(continuousTime * PI);
	float sn = sin(continuousTime * PI);
	
	float2 rotatedUv;
	rotatedUv.x = dot(noiseUv, float2(cs, -sn));
	rotatedUv.y = dot(noiseUv, float2(+sn, cs));

	float4 sampledNoise = noiseTexture.SampleLevel(noiseSampler, rotatedUv, 0.0);

	return 
	// 1.0; 
	// sampledNoise; 
	sampleAmbientOcclusion(fsIn.texCoord0, texelSize, sampledNoise);
}
                                                                