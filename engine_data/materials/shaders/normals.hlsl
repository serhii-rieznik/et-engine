#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 worldTransform;
	row_major float4x4 worldRotationTransform;	
	row_major float4x4 viewProjectionTransform;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.normal = mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz;
	vsOut.position = mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
	return vsOut;
}

struct FSOutput
{
	float4 color0 : SV_Target0;
};

FSOutput fragmentMain(VSOutput fsIn)
{
	FSOutput fsOut;
	fsOut.color0 = float4(0.5 + 0.5 * normalize(fsIn.normal), 1.0);
	return fsOut;
}
