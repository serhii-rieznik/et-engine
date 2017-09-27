#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "moments.h"

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 worldTransform;
	row_major float4x4 viewProjectionTransform;
};

struct VSOutput
{
	float4 position : SV_Position;
#if (ShadowMapping == ShadowMappingMoments)
	float4 projected;
#endif
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput output;
	output.position = mul(mul(float4(vsIn.position, 1.0), worldTransform), viewProjectionTransform);
#if (ShadowMapping == ShadowMappingMoments)
	output.projected = output.position;
#endif
	return output;
}

#if (ShadowMapping == ShadowMappingMoments)

float4 fragmentMain(in VSOutput fsIn) : SV_Target0 { return encodeMoments(fsIn.projected.z); }

#else

void fragmentMain() { } 

#endif



