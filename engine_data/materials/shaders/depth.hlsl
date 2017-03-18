#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 worldTransform;
	row_major float4x4 viewProjectionTransform;
};

float4 vertexMain(VSInput vsIn) : SV_Position
{
	return mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
}

void fragmentMain() { }

