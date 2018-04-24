#include <et>
#include <inputdefines>
#include "atmosphere.h"
#include "environment.h"

#define DRAW_SUN 1
#define SAMPLE_ATMOSPHERE 1

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
	row_major float4x4 inverseViewTransform;
	row_major float4x4 inverseProjectionTransform;
	row_major float4x4 viewProjectionTransform;
	row_major float4x4 inverseViewProjectionTransform;
	float4 lightDirection;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float3 direction : TEXCOORD1;
};

VSOutput vertexMain(uint vertexIndex : SV_VertexID)
{
	float2 pos = float2((vertexIndex << 1) & 2, vertexIndex & 2) * 2.0 - 1.0;

	float4x4 invView = inverseViewTransform;
	invView[3] = float4(0.0, 0.0, 0.0, 1.0);
	float4x4 finalInverseMatrix = mul(inverseProjectionTransform, invView);
	
	VSOutput vsOut;
	vsOut.texCoord = pos * 0.5 + 0.5;
	vsOut.position = float4(pos, 0.000001, 1.0);
	vsOut.direction = mul(float4(pos, 0.0, 1.0), finalInverseMatrix).xyz;
	return vsOut;
}

struct FSOutput 
{
	float4 color : SV_Target0;
	float2 velocity : SV_Target1;
};

FSOutput fragmentMain(VSOutput fsIn)
{
	float3 v = normalize(fsIn.direction);

	float phi = fsIn.texCoord.x * 2.0 * PI;
	float the = PI * (1.0 - fsIn.texCoord.y) - 0.5 * PI;

	float3 light = lightDirection;
	// v = float3(cos(phi) * cos(the), sin(the), sin(phi) * cos(the));

	AtmosphereParameters ap;
	ap.heightAboveGround = HEIGHT_ABOVE_GROUND;
	ap.viewZenithAngle = v.y;
	ap.lightZenithAngle = light.y;

#if (SAMPLE_ATMOSPHERE)
	float3 env = samplePrecomputedAtmosphere(ap, v, light);
#else
	float3 env = sampleEnvironment(v, light.xyz, 0.1);
#endif

#if (DRAW_SUN)
	env += sunColor(v, light.xyz);
#endif
	
	FSOutput result;
	result.color = float4(env, 1.0);
	result.velocity = 0.0;
	return result;
}
