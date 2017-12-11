#include <et>
#include <inputdefines>
#include <options>
#include "srgb.h"

#define LuminanceLowerRange 0.001
#define EnableTemporalAA 0

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
Texture2D<float4> emissiveColorTexture : DECL_TEXTURE(EmissiveColor);
Texture2D<float4> shadowTexture : DECL_TEXTURE(Shadow);
Texture2D<float4> normalTexture : DECL_TEXTURE(Normal);

SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);
SamplerState emissiveColorSampler : DECL_SAMPLER(EmissiveColor);
SamplerState shadowSampler : DECL_SAMPLER(Shadow);
SamplerState normalSampler : DECL_SAMPLER(Normal);

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	float deltaTime;
	float4 cameraJitter;
};

cbuffer MaterialVariables : DECL_BUFFER(Material) 
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
#if (GATHER_AVERAGE)
	float currentLevel = extraParameters.x;
	float sampledLevel = max(0.0, currentLevel - 1);

	float w = 0;
	float h = 0;
	float levels = 0;
	baseColorTexture.GetDimensions(sampledLevel, w, h, levels);

	const float delta = 0.5;
	float4 sX = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, sampledLevel);
	float4 s0 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2(-delta / w, -delta / h), sampledLevel);
	float4 s1 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2( delta / w, -delta / h), sampledLevel);
	float4 s2 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2(-delta / w,  delta / h), sampledLevel);
	float4 s3 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2( delta / w,  delta / h), sampledLevel);
	float4 averageColor = 0.2 * (sX + s0 + s1 + s2 + s3);
#endif

#if (LOG_LUMINANCE)

	float lum = dot(averageColor.xyz, float3(0.2126, 0.7152, 0.0722));
	return log(max(lum, LuminanceLowerRange));

#elif (AVERAGE_LUMINANCE)

	return averageColor;

#elif (RESOLVE_LUMINANCE)

	float previousExposure = shadowTexture.SampleLevel(shadowSampler, float2(0.5, 0.5), 0.0).x;
	float lum = exp(averageColor.x);
	float ev100 = log2(lum * 100.0 / 12.5);
	float exposure = 1.0 / (1.2 * exp2(ev100));
	float adaptationSpeed = lerp(3.0, 5.0, step(exposure - previousExposure, 0.0));
	return lerp(previousExposure, exposure, 1.0f - exp(-deltaTime * adaptationSpeed));

#elif (MOTION_BLUR)

	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	baseColorTexture.GetDimensions(0, w, h, levels);

	const uint maxSamples = 10;
	const float targetDeltaTime = 1.0 / 60.0;
	float velocityScale = 0.5 * min(1.0, deltaTime / targetDeltaTime);

	float2 velocity = velocityScale * emissiveColorTexture.Sample(emissiveColorSampler, fsIn.texCoord0).xy;
	uint currentSamples = clamp(uint(length(velocity * float2(w, h))), 1, maxSamples);

	float totalWeight = 1.0;
	float4 color = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
	//*
	for (uint i = 1; i < currentSamples; ++i)
	{
		float t = float(i - 1) / float(currentSamples - 1);
		float weight = 4.0 * t * (1.0 - t);
		color += weight * baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0 + velocity * (t - 0.5));
		totalWeight += weight;
	}
	// */
	return color / totalWeight;

#elif (TONE_MAPPING)

	float3 source = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, 0.0).xyz;
	float averageLuminance = emissiveColorTexture.SampleLevel(emissiveColorSampler, fsIn.texCoord0, 10.0).x;
	float3 ldrColor = toneMapping(source, averageLuminance);

	/*
	float z0 = floor(ldrColor.z * 16.0) / 16.0;
	float z1 = z0 + 1.0 / 16.0;
	float u = ldrColor.x / 16.0 + 0.5 / 256.0;
	float v = ldrColor.y + 0.5 / 16.0;
	float3 sample0 = shadowTexture.SampleLevel(shadowSampler, float2(u + z0, v), 0.0).xyz;
	float3 sample1 = shadowTexture.SampleLevel(shadowSampler, float2(u + z1, v), 0.0).xyz;
	ldrColor = lerp(sample0, sample1, (ldrColor.z - z0) * 16.0);
	// */
	
	return float4(ldrColor, 1.0);

#elif (TEMPORAL_AA)
	
	const float jitterScale = 0.5;
	float2 tc = cameraJitter.xy * jitterScale + fsIn.texCoord0;
	float2 vel = jitterScale * normalTexture.Sample(normalSampler, tc).xy;

	float2 uvH = tc - vel - jitterScale * cameraJitter.zw;
	float3 sH = emissiveColorTexture.Sample(emissiveColorSampler, uvH).xyz;

	float3 s20 = baseColorTexture.Sample(baseColorSampler, tc, int2(-1, +1)).xyz;
	float3 s21 = baseColorTexture.Sample(baseColorSampler, tc, int2( 0, +1)).xyz;
	float3 s22 = baseColorTexture.Sample(baseColorSampler, tc, int2(+1, +1)).xyz;
	float3 s10 = baseColorTexture.Sample(baseColorSampler, tc, int2(-1,  0)).xyz;
	float3 s11 = baseColorTexture.Sample(baseColorSampler, tc, int2( 0,  0)).xyz;
	float3 s12 = baseColorTexture.Sample(baseColorSampler, tc, int2(+1,  0)).xyz;
	float3 s01 = baseColorTexture.Sample(baseColorSampler, tc, int2( 0, -1)).xyz;
	float3 s00 = baseColorTexture.Sample(baseColorSampler, tc, int2(-1, -1)).xyz;
	float3 s02 = baseColorTexture.Sample(baseColorSampler, tc, int2(+1, -1)).xyz;
	float3 sum0 = 0.25 * (s01 + s10 + s12 + s21);
	float3 sum1 = 0.25 * (s00 + s02 + s20 + s22);

	float3 cMin0 = min(min(s01, s10), min(s12, s21));
	float3 cMax0 = max(max(s01, s10), max(s12, s21));
	float3 cMin1 = min(min(s00, s02), min(s20, s22));
	float3 cMax1 = max(max(s00, s02), max(s20, s22));
	float3 cMin = 0.5 * (cMin0 + min(cMin0, cMin1));
	float3 cMax = 0.5 * (cMax0 + max(cMax0, cMax1));
	
	sH = clamp(sH, cMin, cMax);
	
	float3 wK0 = abs(0.5 * (sum1 + sum0) - s11);
	float3 wK1 = abs(sum0 - s11);
	float3 wK = 0.5 * (wK0 + wK1);
	
	const float3 kLowFreq = 1.33333333;
	const float3 kHighFreq = 0.33333333;
	float3 wRGB = saturate(rcp(lerp(kLowFreq, kHighFreq, wK)));

	// return float4(wRGB, 1.0);
	// return float4(sH, 1.0);
	// return float4(s11, 1.0);
	return float4(lerp(s11, sH, wRGB), 1.0);

#else

	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);

#endif
}
