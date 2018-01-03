#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "moments.h"
#include "common.h"

Texture2D<float4> opacityTexture : DECL_TEXTURE(Opacity);
SamplerState opacitySampler : DECL_SAMPLER(Opacity);

cbuffer ObjectVariables : DECL_BUFFER(Object)
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
#if (HARDCODE_OBJECTS_POSITION)
    float4 transformedPosition = float4(vsIn.position + HARDCODED_OBJECT_POSITION, 1.0);
#else
    float4 transformedPosition = mul(float4(vsIn.position, 1.0), worldTransform);
#endif	

	VSOutput output;
    output.texCoord0 = vsIn.texCoord0;
	output.position = mul(transformedPosition, viewProjectionTransform);
	output.position.xy += cameraJitter.xy * output.position.w;

#if (ShadowMapping == ShadowMappingMoments)
	output.projected = output.position;
#endif

	return output;
}

#if (ShadowMapping == ShadowMappingMoments)

float4 fragmentMain(in VSOutput fsIn) : SV_Target0 
{ 
    float4 opacitySample = opacityTexture.Sample(opacitySampler, fsIn.texCoord0);
    if (opacitySample.x < ALPHA_TEST_TRESHOLD) 
    	discard;
	
	return encodeMoments(fsIn.projected.z); 
}

#else

void fragmentMain(in VSOutput fsIn) 
{
    float4 opacitySample = opacityTexture.Sample(opacitySampler, fsIn.texCoord0);
    if (opacitySample.x < 32.0 / 255.0) 
    	discard;
} 

#endif



