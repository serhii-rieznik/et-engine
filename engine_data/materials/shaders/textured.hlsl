#include <et>
#include <inputdefines>
#include <inputlayout>

Texture2D<float4> inputTexture : DECLARE_TEXTURE;

cbuffer ObjectVariables : DECL_OBJECT_BUFFER 
{
	row_major float4x4 worldTransform; 
	row_major float4x4 viewProjectionTransform;
};

cbuffer MaterialVariables : DECL_MATERIAL_BUFFER 
{
	float4 extraParameters;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

#define USE_INPUT_VERTICES (TRANSFORM_2D_POSITION || TRANSFORM_INPUT_POSITION)

#if (USE_INPUT_VERTICES)
VSOutput vertexMain(VSInput vsIn) {
	float2 pos = vsIn.position;
#else
VSOutput vertexMain(uint vertexIndex : SV_VertexID) { 
	float2 pos = float2((vertexIndex << 1) & 2, vertexIndex & 2) * 2.0 - 1.0;
#endif

	VSOutput vsOut;
	vsOut.texCoord0 = pos * 0.5 + 0.5;
	vsOut.position = float4(pos, 0.0, 1.0);

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
	float sample = inputTexture.SampleLevel(PointClamp, fsIn.texCoord0, 0.0).x;
	return pow(sample, 4.0);
#elif (SPECIFIC_LOD)
	return inputTexture.SampleLevel(LinearClamp, fsIn.texCoord0, extraParameters.x);
#else
	return inputTexture.Sample(LinearClamp, fsIn.texCoord0);
#endif
}
