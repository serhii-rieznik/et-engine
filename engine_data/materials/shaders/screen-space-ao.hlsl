#include <et>
#include <inputdefines>
#include <options>

Texture2D<float> sceneDepth : DECLARE_TEXTURE;
Texture2D<float4> noise : DECLARE_TEXTURE;

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
    row_major float4x4 viewTransform;
    row_major float4x4 inverseViewTransform;
    row_major float4x4 projectionTransform;
    row_major float4x4 inverseProjectionTransform;
    row_major float4x4 viewProjectionTransform;
    row_major float4x4 inverseViewProjectionTransform;
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
    float4 a = mul(float4(uv, z, 1.0), inverseProjectionTransform);
    return a.xyz / a.w;
}

float3 projectionSpace(in float3 coord)
{
	float2 uvScale = float2(-inverseProjectionTransform[0][0], -inverseProjectionTransform[1][1]);
	float encodedZ = (cameraClipPlanes.x * cameraClipPlanes.y / coord.z + cameraClipPlanes.y) / (cameraClipPlanes.y - cameraClipPlanes.x);
	return float3(coord.xy / (coord.z * uvScale), encodedZ);
}
                    
static const float aoScale = 1.0;

static const uint directionsCount = 8;

static const float3 predefinedDirection[directionsCount] = {
    float3(1.0, 0.0, 0.0),
    float3(0.707107, 0.707107, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(-0.707107, 0.707107, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(-0.707107, -0.707107, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.707107, -0.707107, 0.0),
};

float3 getRotatedDirection(in uint index, in float4 m)
{
	float2 base = predefinedDirection[index];
	return float3(dot(base, m.xy), dot(base, m.zw), 0.0);
}

float3 reconstructNormal(in float2 uv, in float2 texelSize)
{
    float2 uv_left = uv - float2(texelSize.x, 0.0);
    float z_left = sceneDepth.Sample(PointClamp, uv_left);
    float3 p_left = viewSpace(uv_left * 2.0 - 1.0, z_left);
    
    float2 uv_right = uv + float2(texelSize.x, 0.0);
    float z_right = sceneDepth.Sample(PointClamp, uv_right);
    float3 p_right = viewSpace(uv_right * 2.0 - 1.0, z_right);

    float2 uv_top = uv - float2(0.0, texelSize.y);
    float z_top = sceneDepth.Sample(PointClamp, uv_top);
    float3 p_top = viewSpace(uv_top * 2.0 - 1.0, z_top);
    
    float2 uv_bottom = uv + float2(0.0, texelSize.y);
    float z_bottom = sceneDepth.Sample(PointClamp, uv_bottom);
    float3 p_bottom = viewSpace(uv_bottom * 2.0 - 1.0, z_bottom);

    return normalize(cross(p_right - p_left, p_top - p_bottom));
}

float4 sampleAmbientOcclusion(in float2 uv, in float2 texelSize, in float4 rnd)
{
	float3 p0 = viewSpace(uv * 2.0 - 1.0, sceneDepth.Sample(PointClamp, uv));
	float3 n0 = reconstructNormal(uv, texelSize);

	const uint samplesPerDirection = 8;

	float influenceRadius = 1.0 * (rnd.x * 0.75 + 0.25);

	float directionStepSize = influenceRadius / (p0.z * float(samplesPerDirection));

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
			float3 stepViewSpace = viewSpace(stepUv * 2.0 - 1.0, sceneDepth.Sample(PointClamp, stepUv));
			float3 horizonVector = stepViewSpace - p0;
			float horizonVectorLength = length(horizonVector);
			float attenuation = saturate(1.0 - horizonVectorLength / influenceRadius);
			ao += attenuation * saturate(dot(n0, horizonVector) / horizonVectorLength);
			stepUv += projectedStep;
		}
	}
    ao = saturate(1.0 - ao / float(directionsCount * samplesPerDirection));
   
    ao = pow(ao, 16.0);
    
	return ao;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float w = 0.0f;
	float h = 0.0f;
	float levels = 0.0;
	sceneDepth.GetDimensions(0, w, h, levels);

	float2 texelSize = 1.0 / float2(w, h);

	w = 0.0f;
	h = 0.0f;
	noise.GetDimensions(0, w, h, levels);
	float2 noiseTexelSize = float2(w, h);

	float2 noiseUv = fsIn.texCoord0 * (viewport.zw / noiseTexelSize);
	
	float cs = cos(continuousTime * PI);
	float sn = sin(continuousTime * PI);
	
	float2 rotatedUv = noiseUv;
	rotatedUv.x = dot(noiseUv, float2(cs, -sn));
	rotatedUv.y = dot(noiseUv, float2(+sn, cs));

	float4 sampledNoise = noise.SampleLevel(PointWrap, rotatedUv, 0.0);

	return 
	  // 1.0; 
	  // sampledNoise; 
	  sampleAmbientOcclusion(fsIn.texCoord0, texelSize, sampledNoise);
}
                                                                