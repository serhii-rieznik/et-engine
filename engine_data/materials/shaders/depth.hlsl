#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "moments.h"
#include "common.h"

Texture2D<float4> opacity : DECLARE_TEXTURE;

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
	row_major float4x4 worldTransform;
	row_major float4x4 viewProjectionTransform;
	float4 cameraJitter;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 texCoord0;
#if (ShadowMapping == ShadowMappingMoments)
	float4 projected;
#endif
};

VSOutput vertexMain(VSInput vsIn)
{
    float4 transformedPosition = mul(float4(vsIn.position, 1.0), worldTransform);

	VSOutput output;
    output.texCoord0 = vsIn.texCoord0;
	output.position = mul(transformedPosition, viewProjectionTransform);

#if (ShadowMapping == ShadowMappingMoments)
	output.projected = output.position;
#endif

	return output;
}

#if ((DEPTH_PREPASS) || (ShadowMapping != ShadowMappingMoments))

void fragmentMain(in VSOutput fsIn) 
{
    float4 opacitySample = opacity.Sample(LinearWrap, fsIn.texCoord0);
    if (opacitySample.x < 32.0 / 255.0) 
    	discard;
} 

#elif (ShadowMapping == ShadowMappingMoments)

float4 fragmentMain(in VSOutput fsIn) : SV_Target0 
{ 
    float4 opacitySample = opacity.Sample(LinearWrap, fsIn.texCoord0);
    if (opacitySample.x < ALPHA_TEST_TRESHOLD) 
    	discard;
	
	return encodeMoments(fsIn.projected.z); 
}

#else

#	error Invalid Configuration

#endif



