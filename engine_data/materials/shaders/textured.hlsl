#include <et>
#include <inputdefines>
#include <inputlayout>

#if (TRANSFORM_INPUT_POSITION || TRANSFORM_2D_POSITION)
cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex) 
{
	row_major float4x4 worldTransform; 
};
#endif

Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
SamplerState baseColorSampler : CONSTANT_LOCATION(s, BaseColorSamplerBinding, TexturesSetIndex);

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

#if (TRANSFORM_INPUT_POSITION)
	vsOut.position = mul(mul(vsOut.position, worldTransform), viewProjection);
#elif (TRANSFORM_2D_POSITION)
	vsOut.position = mul(vsOut.position, worldTransform);
#endif

	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
}
