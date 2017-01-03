#include <et>
#include <inputdefines>
#include <inputlayout>

#if (TRANSFORM_INPUT_POSITION)
cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex) 
{
	float4x4 worldTransform; 
};
#endif

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;

#if (TRANSFORM_INPUT_POSITION)
	vsOut.position = mul(mul(float4(position, 1.0), worldTransform), viewProjection);
#else
	vsOut.position = float4(vsIn.position, 1.0);
#endif

	return vsOut;
}

Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
SamplerState baseColorSampler;

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
}
