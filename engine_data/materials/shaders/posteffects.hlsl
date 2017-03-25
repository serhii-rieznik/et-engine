#include <et>
#include <inputdefines>
#include <inputlayout>
#include "srgb.h"

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

Texture2D<float4> emissiveColorTexture : DECL_TEXTURE(EmissiveColor);
SamplerState emissiveColorSampler : DECL_SAMPLER(EmissiveColor);

Texture2D<float4> shadowTexture : DECL_TEXTURE(Shadow);
SamplerState shadowSampler : DECL_SAMPLER(Shadow);

cbuffer MaterialVariables : DECL_BUFFER(Material) 
{
	float extraParameters;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.position = float4(vsIn.position, 1.0);
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
#if (LUMINANCE_DOWNSAMPLE)

	#define delta 0.5
	float currentLevel = extraParameters.x;
	float previousLevel = max(0.0, currentLevel - 1.0);

	float w = 0;
	float h = 0;
	float levels = 0;
	baseColorTexture.GetDimensions(previousLevel, w, h, levels);

	float4 sX = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, previousLevel);
	float4 s0 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2(-delta / w, -delta / h), previousLevel);
	float4 s1 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2( delta / w, -delta / h), previousLevel);
	float4 s2 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2(-delta / w,  delta / h), previousLevel);
	float4 s3 = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0 + float2( delta / w,  delta / h), previousLevel);
	float4 average = 0.2 * (sX + s0 + s1 + s2 + s3);
	
	if (currentLevel == 0.0)
	{
		return log2(max(dot(average.xyz, float3(0.2989, 0.5870, 0.1140)), 0.00001));
	}

	if (currentLevel + 1.0 >= levels)
	{
		float previousLuminance = shadowTexture.SampleLevel(shadowSampler, float2(0.5, 0.5), 0.0).x;
		float expoCorrection = 0.0;
		float ev100 = log2(exp(average.x) * 100.0 / 12.5) - expoCorrection;
		return lerp(previousLuminance, 0.18 / (0.125 * pow(2.0, ev100)), 0.5);
	}

	return average;

#elif (RESOLVE)

	float3 source = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0).xyz;
	float exposure = emissiveColorTexture.SampleLevel(emissiveColorSampler, fsIn.texCoord0, 10.0).x;
	return float4(toneMapping(source, exposure), 1.0);

#else

	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);

#endif
}
