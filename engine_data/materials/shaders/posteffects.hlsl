#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

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

	#define delta 1.0
	float previousLevel = extraParameters.x - 1.0;
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

	if (previousLevel == 0.0)
		return log2(max(dot(average.xyz, float3(0.2989, 0.5870, 0.1140)), 0.00001));

	if (extraParameters.x >= levels - 1)
		return exp(average);

	return average;

#else

	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);

#endif
}
