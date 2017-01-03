#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex)
{
	float4x4 worldTransform;
};

float4 vertexMain(VSInput vsIn) : SV_Position
{
	return mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjection);
}

void fragmentMain() { }

