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
	float velocityScale = min(1.0, deltaTime / targetDeltaTime);

	float2 velocity = normalTexture.Sample(emissiveColorSampler, fsIn.texCoord0).xy;
	uint currentSamples = 1; // clamp(uint(length(velocity * float2(w, h))), 1, maxSamples);

	float2 currentUv = fsIn.texCoord0;
	float2 previousUv = fsIn.texCoord0 - velocityScale * velocity;

	float4 color = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
	for (uint i = 1; i < currentSamples; ++i)
	{
		float t = float(i) / float(currentSamples - 1);
		float2 uv = lerp(currentUv, previousUv, t);
		color += baseColorTexture.Sample(baseColorSampler, uv);
	}

	return color / float(currentSamples);

#elif (TONE_MAPPING)

	float3 source = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, 0.0).xyz;
	float averageLuminance = emissiveColorTexture.SampleLevel(emissiveColorSampler, fsIn.texCoord0, 10.0).x;
	float3 ldrColor = toneMapping(source, averageLuminance);

#	if (ENABLE_COLOR_GRADING)
	{
		float z0 = floor(ldrColor.z * 16.0) / 16.0;
		float z1 = z0 + 1.0 / 16.0;
		float u = ldrColor.x / 16.0 + 0.5 / 256.0;
		float v = ldrColor.y + 0.5 / 16.0;
		float3 sample0 = shadowTexture.SampleLevel(shadowSampler, float2(u + z0, v), 0.0).xyz;
		float3 sample1 = shadowTexture.SampleLevel(shadowSampler, float2(u + z1, v), 0.0).xyz;
		ldrColor = lerp(sample0, sample1, (ldrColor.z - z0) * 16.0);
	}
#	endif
	
	return float4(ldrColor, 1.0);

#elif (TEMPORAL_AA)
	
	float2 baseUv = fsIn.texCoord0 + 0.5 * cameraJitter.xy;
	float2 velocity = normalTexture.Sample(baseColorSampler, baseUv).xy;

	float4 currentSample = baseColorTexture.Sample(baseColorSampler, baseUv);

	float2 motion = 0.5 * (cameraJitter.xy - cameraJitter.zw);
	float2 historyUv = fsIn.texCoord0 + velocity.xy;// - motion;
	float4 historySample = emissiveColorTexture.Sample(emissiveColorSampler, historyUv);

	float4 box[9];
	box[0] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(-1, -1));
	box[1] = baseColorTexture.Sample(baseColorSampler, baseUv, int2( 0, -1));
	box[2] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(+1, -1));
	box[3] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(-1,  0));
	box[4] = currentSample;
	box[5] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(+1,  0));
	box[6] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(-1, +1));
	box[7] = baseColorTexture.Sample(baseColorSampler, baseUv, int2( 0, +1));
	box[8] = baseColorTexture.Sample(baseColorSampler, baseUv, int2(+1, +1));

	float4 minValue = min(min(box[0], box[1]), min(min(box[2], box[3]), 
		min(min(box[4], box[5]), min(min(box[6], box[7]), box[8]))));

	float4 maxValue = max(max(box[0], box[1]), max(max(box[2], box[3]), 
		max(max(box[4], box[5]), max(max(box[6], box[7]), box[8]))));

	historySample = clamp(historySample, minValue, maxValue);

	float currentLuminance = dot(currentSample.xyz, float3(0.2126, 0.7152, 0.0722));
	float historyLuminance = dot(historySample.xyz, float3(0.2126, 0.7152, 0.0722));

	const float diffMinDivisor = 0.1; // 0.25;
	const float lerpMinValue = 0.125;// / 8.0; // 1.0 / 3.0;
	const float lerpMaxValue = 0.875; // 2.0 / 3.0;

	float weight = 1.0 - abs(currentLuminance - historyLuminance) / max(diffMinDivisor, max(currentLuminance, historyLuminance));
	float lerpValue = lerp(lerpMinValue, lerpMaxValue, weight * weight);

	// lerpValue = 0.0;

	return 
	// length(velocity.xy - motion) * 1000.0;
	// lerpValue;
	// weight;
	// historySample;
	// currentSample;
	lerp(historySample, currentSample, lerpValue);

#else

	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);

#endif
}
