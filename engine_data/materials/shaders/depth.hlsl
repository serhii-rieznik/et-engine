#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "moments.h"
#include "common.h"

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
#if (HARDCODE_OBJECTS_POSITION)
    float4 transformedPosition = float4(vsIn.position + HARDCODED_OBJECT_POSITION, 1.0);
#else
    float4 transformedPosition = mul(float4(vsIn.position, 1.0), worldTransform);
#endif	

	VSOutput output;
	output.position = mul(transformedPosition, viewProjectionTransform);
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



