#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "srgb.h"

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

cbuffer MaterialVariables : DECL_BUFFER(Material) 
{
	float4 extraParameters;
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
	static const int radius = 4;

	if (radius == 0)
		return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);

	float3 textureSize = 0.0;
	baseColorTexture.GetDimensions(0, textureSize.x, textureSize.y, textureSize.z);

	float2 texel = extraParameters.xy / textureSize.xy;
	float2 coord = fsIn.texCoord0 - float(radius) * texel;
	
	float weight = 0.0;
	float4 result = 0.0;
	for (int i = -radius; i <= radius; ++i)
	{
		float w = 1.0 - abs(float(i) / float(radius));
		weight += w;
		result += w * baseColorTexture.Sample(baseColorSampler, coord);
		coord += texel;
	}
	
	return result / weight;
}
