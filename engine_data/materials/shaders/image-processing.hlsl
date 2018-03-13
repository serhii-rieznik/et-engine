#include <et>
#include <inputdefines>
#include <options>
#include "srgb.h"

Texture2D<float4> inputTexture : DECLARE_TEXTURE;

cbuffer MaterialVariables : DECL_MATERIAL_BUFFER 
{
	float4 extraParameters;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

#include "vertexprogram-2d-triangle.h"

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	static const int radius = 4;

	if (radius == 0)
		return inputTexture.Sample(LinearClamp, fsIn.texCoord0);

	float3 textureSize = 0.0;
	inputTexture.GetDimensions(0, textureSize.x, textureSize.y, textureSize.z);

	float2 texel = extraParameters.xy / textureSize.xy;
	float2 coord = fsIn.texCoord0 - float(radius) * texel;
	
	float weight = 0.0;
	float4 result = 0.0;
	for (int i = -radius; i <= radius; ++i)
	{
		float w = 1.0 - abs(float(i) / float(radius));
		weight += w;
		result += w * inputTexture.Sample(LinearClamp, coord);
		coord += texel;
	}
	
	return result / weight;
}
