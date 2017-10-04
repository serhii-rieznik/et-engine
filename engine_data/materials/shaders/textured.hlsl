#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

cbuffer ObjectVariables : DECL_BUFFER(Object) 
{
	row_major float4x4 worldTransform; 
	row_major float4x4 viewProjectionTransform;
};

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
	vsOut.texCoord0 = vsIn.position.xy * 0.5 + 0.5;
	vsOut.position = float4(vsIn.position, 1.0);

#if (TRANSFORM_INPUT_POSITION)
	vsOut.position = mul(mul(vsOut.position, worldTransform), viewProjectionTransform);
#elif (TRANSFORM_2D_POSITION)
	vsOut.position = mul(vsOut.position, worldTransform);
#endif

	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
#if (DISPLAY_DEPTH)
	float sample = baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, 0.0).x;
	return pow(sample, 1.0);
#elif (SPECIFIC_LOD)
	return baseColorTexture.SampleLevel(baseColorSampler, fsIn.texCoord0, extraParameters.x);
#else
	return baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
#endif
}
