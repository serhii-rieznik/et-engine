#include <et>
#include <inputdefines>
#include "atmosphere.h"
#include "environment.h"

#define DRAW_SUN 1
#define SAMPLE_ATMOSPHERE 0

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
	row_major float4x4 inverseViewTransform;
	row_major float4x4 inverseProjectionTransform;
	row_major float4x4 viewProjectionTransform;
	row_major float4x4 inverseViewProjectionTransform;
	float4 lightDirection;
	float3 lightColor;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
	float3 direction : TEXCOORD1;
};

VSOutput vertexMain(uint vertexIndex : SV_VertexID)
{
	float2 pos = float2((vertexIndex << 1) & 2, vertexIndex & 2) * 2.0 - 1.0;

	float4x4 invView = inverseViewTransform;
	invView[3] = float4(0.0, 0.0, 0.0, 1.0);
	float4x4 finalInverseMatrix = mul(inverseProjectionTransform, invView);
	
	VSOutput vsOut;
	vsOut.texCoord0 = pos * 0.5 + 0.5;
	vsOut.position = float4(pos, 0.9999999, 1.0);
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

#if (SAMPLE_ATMOSPHERE)
	float3 env = sampleAtmosphere(v, lightDirection.xyz, lightColor);
#else
	float3 env = sampleEnvironment(v, lightDirection.xyz, 0.1);
#endif

#if (DRAW_SUN)
	float intersectsPlanet = step(planetIntersection(positionOnPlanet, v), 0.0);
	float sunSpot = smoothstep(0.9995, 0.99975, dot(v, lightDirection.xyz));
	float3 sunColor = lightColor * (sunSpot * intersectsPlanet);
	float a = atmosphereIntersection(positionOnPlanet, lightDirection.xyz);
	env += sunColor * outScattering(positionOnPlanet, positionOnPlanet + a * lightDirection.xyz);
#endif
	
	FSOutput result;
	result.color = float4(env, 1.0);
	result.velocity = 0.0;
	return result;
}
