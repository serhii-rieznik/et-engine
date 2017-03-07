#include <et>
#include <inputdefines>
#include <inputlayout>

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
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	return float4(fsIn.texCoord0, 0.0, 1.0); // baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
}
