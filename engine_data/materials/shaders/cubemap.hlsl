#include <et>
#include <inputdefines>
#include <inputlayout>

#if (VISUALIZE_CUBEMAP)
	TextureCube<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
#elif (EQ_MAP_TO_CUBEMAP)
	Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
#endif
SamplerState baseColorSampler : CONSTANT_LOCATION(s, BaseColorSamplerBinding, TexturesSetIndex);

cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex)
{
	row_major float4x4 worldTransform;
};

struct VSOutput 
{
	float4 position : SV_Position;

#if (VISUALIZE_CUBEMAP)
	float2 texCoord0 : TEXCOORD0;
#elif (EQ_MAP_TO_CUBEMAP)
	float3 direction : TEXCOORD1;
#endif
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.position = float4(vsIn.position, 1.0);

#if (VISUALIZE_CUBEMAP)
	vsOut.position = mul(vsOut.position, worldTransform);
	vsOut.texCoord0 = vsIn.texCoord0;
#elif (EQ_MAP_TO_CUBEMAP)
	vsOut.direction = mul(vsOut.position, worldTransform).xyz;
#endif

	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
#if (VISUALIZE_CUBEMAP)	
	float phi = fsIn.texCoord0.x * 2.0 * PI - PI;
	float theta = fsIn.texCoord0.y * PI - 0.5 * PI;
	float sinTheta = sin(theta);
	float cosTheta = cos(theta);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);
	float3 sampleDirection = float3(cosPhi * cosTheta, sinTheta, sinPhi * cosTheta);
	return baseColorTexture.Sample(baseColorSampler, sampleDirection);
#elif (EQ_MAP_TO_CUBEMAP)
	float3 d = normalize(fsIn.direction);        	
	float u = atan2(d.z, d.x) * 0.5 / PI + 0.5;
	float v = asin(d.y) / PI + 0.5;
	return baseColorTexture.Sample(baseColorSampler, float2(u, v));
#else
	return 1.0;
#endif
}
