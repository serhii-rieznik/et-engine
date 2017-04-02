#include <et>
#include <inputdefines>
#include <inputlayout>
#include "atmosphere.h"
#include "environment.h"

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 inverseViewTransform;
	row_major float4x4 inverseProjectionTransform;
	float3 lightColor;
	float4 lightDirection;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
	float3 direction : TEXCOORD1;
};

VSOutput vertexMain(VSInput vsIn)
{
	float4x4 invView = inverseViewTransform;
	invView[3] = float4(0.0, 0.0, 0.0, 1.0);
	float4x4 finalInverseMatrix = invView * inverseProjectionTransform;

	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.position = float4(vsIn.position.xy, 0.9999999, 1.0);
	vsOut.direction = mul(float4(vsIn.position, 1.0), finalInverseMatrix).xyz;
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float3 v = normalize(fsIn.direction);

	float intersectsPlanet = step(planetIntersection(positionOnPlanet, v), 0.0);
	float sunSpot = smoothstep(0.9995, 0.99975, dot(v, lightDirection.xyz));
	float3 sunColor = lightColor * (sunSpot * intersectsPlanet);

	float a = atmosphereIntersection(positionOnPlanet, lightDirection.xyz);
	sunColor *= outScattering(positionOnPlanet, positionOnPlanet + a * lightDirection.xyz);

	float3 env = sampleEnvironment(normalize(fsIn.direction), lightDirection.xyz, 0.0);
	// env = sampleAtmosphere(v, lightDirection.xyz, lightColor);
	return float4(env + sunColor, 1.0);
}
