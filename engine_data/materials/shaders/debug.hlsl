#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
	row_major float4x4 worldTransform;
	row_major float4x4 viewProjectionTransform;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float4 color;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.color = vsIn.color;
	vsOut.position = mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
	return vsOut;
}

struct FSOutput
{
	float4 color0 : SV_Target0;
	float4 color1 : SV_Target1;
};

FSOutput fragmentMain(VSOutput fsIn)
{
	FSOutput fsOut;
	fsOut.color0 = fsIn.color;
	fsOut.color1 = 0.0;
	return fsOut;
}
