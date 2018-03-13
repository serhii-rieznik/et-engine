#include <et>
#include <inputdefines>
#include <options>
#include "srgb.h"

#define LuminanceLowerRange 0.0001

Texture2D<float4> inputTexture : DECLARE_TEXTURE;
Texture2D<float4> velocityTexture : DECLARE_TEXTURE;
Texture2D<float4> historyTexture : DECLARE_TEXTURE;
Texture2D<float4> averageLuminance : DECLARE_TEXTURE;

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
	float deltaTime;
	float4 cameraJitter;
};

cbuffer MaterialVariables : DECL_MATERIAL_BUFFER 
{
	float extraParameters;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

#include "vertexprogram-2d-triangle.h"

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float w = 0;
	float h = 0;
	float levels = 0;

#if (GATHER_AVERAGE)
	float currentLevel = extraParameters.x;
	float sampledLevel = max(0.0, currentLevel - 1);
	inputTexture.GetDimensions(sampledLevel, w, h, levels);

	float delta = 0.5;
	float4 sX = inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0, sampledLevel);
	float4 s0 = inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0 + float2(-delta / w, -delta / h), sampledLevel);
	float4 s1 = inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0 + float2( delta / w, -delta / h), sampledLevel);
	float4 s2 = inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0 + float2(-delta / w,  delta / h), sampledLevel);
	float4 s3 = inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0 + float2( delta / w,  delta / h), sampledLevel);
	float4 averageColor = 0.2 * (sX + s0 + s1 + s2 + s3);
#endif

#if (LOG_LUMINANCE)

	float lum = dot(averageColor.xyz, float3(0.2126, 0.7152, 0.0722));
	return log(max(lum, LuminanceLowerRange));

#elif (AVERAGE_LUMINANCE)

	return averageColor;

#elif (MOTION_BLUR)

	inputTexture.GetDimensions(0.0, w, h, levels);

	const uint maxSamples = 11;
	const float targetDeltaTime = 1.0 / 130.0;
	float velocityScale = min(1.0, deltaTime / targetDeltaTime);

	float2 velocity = velocityTexture.Sample(LinearClamp, fsIn.texCoord0);
	uint currentSamples = clamp(uint(length(velocity * float2(w, h))), 1, maxSamples);

	float2 currentUv = fsIn.texCoord0;
	float2 previousUv = fsIn.texCoord0 - velocityScale * velocity;

	float4 color = inputTexture.Sample(LinearClamp, fsIn.texCoord0);
	//*
	for (uint i = 1; i < currentSamples; ++i)
	{
		float t = float(i) / float(currentSamples - 1);
		float2 uv = lerp(currentUv, previousUv, t);
		color += inputTexture.Sample(LinearClamp, uv);
	}
	// */
	return color / float(currentSamples);

#elif (TONE_MAPPING)

	float averageLuminance = averageLuminance.SampleLevel(LinearClamp, fsIn.texCoord0, 10.0).x;

	inputTexture.GetDimensions(0.0, w, h, levels);
	float2 texel = float2(1.0 / w, 1.0 / h);

	float3 hdrColor = inputTexture.Sample(LinearClamp, fsIn.texCoord0).xyz;
	float3 ldrColor = toneMapping(hdrColor, averageLuminance, fsIn.texCoord0.x);

#	if (ENABLE_COLOR_GRADING)
	{
		float z0 = floor(ldrColor.z * 16.0) / 16.0;
		float z1 = z0 + 1.0 / 16.0;
		float u = ldrColor.x / 16.0 + 0.5 / 256.0;
		float v = ldrColor.y + 0.5 / 16.0;
		float3 sample0 = colorGradingTable.SampleLevel(LinearClamp, float2(u + z0, v), 0.0).xyz;
		float3 sample1 = colorGradingTable.SampleLevel(LinearClamp, float2(u + z1, v), 0.0).xyz;
		ldrColor = lerp(sample0, sample1, (ldrColor.z - z0) * 16.0);
	}
#	endif
	
	return float4(ldrColor, 1.0);

#elif (TEMPORAL_AA)
	
	float2 velocitySample = velocityTexture.Sample(LinearClamp, fsIn.texCoord0);
	float3 historySample = historyTexture.Sample(LinearClamp, fsIn.texCoord0 + velocitySample).xyz;

	float3 s00 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(-1, -1)).xyz;
	float3 s01 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2( 0, -1)).xyz;
	float3 s02 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(+1, -1)).xyz;
	float3 s10 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(-1, 0)).xyz;
	float3 s11 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2( 0, 0)).xyz;
	float3 s12 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(+1, 0)).xyz;
	float3 s20 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(-1, +1)).xyz;
	float3 s21 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2( 0, +1)).xyz;
	float3 s22 = inputTexture.Sample(LinearClamp, fsIn.texCoord0, uint2(+1, +1)).xyz;

	float3 minSample = min(s00, min(min(min(s01, s02), min(s10, s11)), min(min(s12, s20), min(s21, s22))));
	float3 maxSample = max(s00, max(max(max(s01, s02), max(s10, s11)), max(max(s12, s20), max(s21, s22))));
	float3 average = 1.0 / 9.0 * (s00 + s01 + s02 + s10 + s11 + s12 + s20 + s21 + s22);

	minSample = RGBToYCoCg(minSample);
	maxSample = RGBToYCoCg(maxSample);
	historySample = RGBToYCoCg(historySample);
	historySample = clamp(historySample, minSample, maxSample);
	float3 currentSample = RGBToYCoCg(s11);

	float weightMin = 0.5;
	float weightMax = 0.15;
	float lumDifference = abs(currentSample.y - historySample.y) / max(currentSample.y, max(historySample.y, 0.2));
	float weight = 1.0 - lumDifference;
	float weightSquared = weight * weight;
	weight = lerp(weightMin, weightMax, weightSquared);
	float3 temporal = lerp(historySample, currentSample, weight);

	return float4(YCoCgToRGB(temporal), 1.0);

#else

	return inputTexture.Sample(LinearClamp, fsIn.texCoord0);

#endif
}
