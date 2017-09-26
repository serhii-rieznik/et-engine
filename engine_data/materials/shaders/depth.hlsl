#include <et>
#include <inputdefines>
#include <inputlayout>
#include "options.h"
#include "shadowmapping.h"

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 worldTransform;
	row_major float4x4 viewProjectionTransform;
};

#if (UseMomentsShadowmap)

struct VSOutput
{
	float4 position : SV_Position;
	float4 projected;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput output;
	output.projected = mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
	output.position = output.projected;
	return output;
}

float4 fragmentMain(in VSOutput fsIn) : SV_Target0 
{
	return encodeMoments(fsIn.projected.z);
}

#else

float4 vertexMain(VSInput vsIn) : SV_Position {
	return mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
}

void fragmentMain() { 
} 

#endif



