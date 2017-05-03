#include <et>
#include <inputdefines>
#include <inputlayout>
#include "atmosphere.h"
#include "environment.h"

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 inverseViewTransform;
	row_major float4x4 previousInverseViewTransform;
	row_major float4x4 inverseProjectionTransform;
	row_major float4x4 previousInverseProjectionTransform;
	float3 lightColor;
	float4 lightDirection;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
	float3 currentDirection : TEXCOORD1;
	float3 previousDirecion : TEXCOORD2;
};

VSOutput vertexMain(VSInput vsIn)
{
	float4x4 invView = inverseViewTransform;
	invView[3] = float4(0.0, 0.0, 0.0, 1.0);
	float4x4 finalInverseMatrix = invView * inverseProjectionTransform;

	invView = previousInverseViewTransform;
	invView[3] = float4(0.0, 0.0, 0.0, 1.0);
	float4x4 previousFinalInverseMatrix = invView * previousInverseProjectionTransform;
	
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.position = float4(vsIn.position.xy, 0.9999999, 1.0);
	vsOut.currentDirection = mul(float4(vsIn.position, 1.0), finalInverseMatrix).xyz;
	vsOut.previousDirecion = mul(float4(vsIn.position, 1.0), previousFinalInverseMatrix).xyz;
	return vsOut;
}

struct FSOutput 
{
	float4 color : SV_Target0;
	float2 velocity : SV_Target1;
};

FSOutput fragmentMain(VSOutput fsIn)
{
	float3 v = normalize(fsIn.currentDirection);
	float3 p = normalize(fsIn.previousDirecion);

	float intersectsPlanet = step(planetIntersection(positionOnPlanet, v), 0.0);
	float sunSpot = smoothstep(0.9995, 0.99975, dot(v, lightDirection.xyz));
	float3 sunColor = lightColor * (sunSpot * intersectsPlanet);

	float a = atmosphereIntersection(positionOnPlanet, lightDirection.xyz);
	sunColor *= outScattering(positionOnPlanet, positionOnPlanet + a * lightDirection.xyz);

	float3 env = sampleEnvironment(v, lightDirection.xyz, 0.0);
	// env = sampleAtmosphere(v, lightDirection.xyz, lightColor);
	
	FSOutput result;
	result.color = float4(env + sunColor, 1.0);
	result.velocity = v.xy - p.xy;
	return result;
}
