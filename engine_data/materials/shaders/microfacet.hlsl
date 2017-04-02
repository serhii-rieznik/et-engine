#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer MaterialVariables : DECL_BUFFER(Material)
{
	float4 diffuseReflectance;
	float4 specularReflectance;
	float4 emissiveColor;
	float roughnessScale;
	float metallnessScale;
};

cbuffer ObjectVariables : DECL_BUFFER(Object)
{
	row_major float4x4 viewProjectionTransform;
	row_major float4x4 worldTransform;
	row_major float4x4 worldRotationTransform;
	row_major float4x4 lightProjectionTransform;
	float3 lightColor;
	float4 lightDirection;
	float4 cameraPosition;
};

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

Texture2D<float4> normalTexture : DECL_TEXTURE(Normal);
SamplerState normalSampler : DECL_SAMPLER(Normal);

Texture2D<float4> shadowTexture : DECL_TEXTURE(Shadow);
SamplerState shadowSampler : DECL_SAMPLER(Shadow);

Texture2D<float4> brdfLookupTexture : DECL_TEXTURE(BrdfLookup);
SamplerState brdfLookupSampler : DECL_SAMPLER(BrdfLookup);

Texture2D<float4> opacityTexture : DECL_TEXTURE(Opacity);
SamplerState opacitySampler : DECL_SAMPLER(Opacity);

struct VSOutput 
{
	float4 position : SV_Position;
	float3 normal;
	float3 toLight;
	float3 toCamera;
	float2 texCoord0;
	float4 lightCoord;
	float3 invTransformT;
	float3 invTransformB;
	float3 invTransformN;
};

VSOutput vertexMain(VSInput vsIn)
{
	float4 transformedPosition = mul(float4(20.0 * vsIn.position, 1.0), worldTransform);
	
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.normal = normalize(mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz);

	float3 tTangent = normalize(mul(float4(vsIn.tangent, 0.0), worldRotationTransform).xyz);
	float3 tBiTangent = cross(vsOut.normal, tTangent);

	vsOut.toCamera = (cameraPosition.xyz - transformedPosition.xyz).xyz;
	vsOut.toLight = (lightDirection.xyz - transformedPosition.xyz * lightDirection.w).xyz;
	vsOut.lightCoord = mul(transformedPosition, lightProjectionTransform);
	vsOut.position = mul(transformedPosition, viewProjectionTransform);
	vsOut.invTransformT = float3(tTangent.x, tBiTangent.x, vsOut.normal.x);
	vsOut.invTransformB = float3(tTangent.y, tBiTangent.y, vsOut.normal.y);
	vsOut.invTransformN = float3(tTangent.z, tBiTangent.z, vsOut.normal.z);
	return vsOut;
}

#include "srgb.h"
#include "bsdf.h"
#include "environment.h"
#include "atmosphere.h"
#include "importance-sampling.h"

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float4 normalSample = normalTexture.Sample(normalSampler, fsIn.texCoord0);
	float4 baseColorSample = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
	float4 opacitySample = opacityTexture.Sample(opacitySampler, fsIn.texCoord0);

	/*
	baseColorSample.xyz = float3(1, 1, 1) * 0.18;
	baseColorSample.w = 1.0;
	normalSample.xyz = float3(0.5, 0.5, 1);
	normalSample.w = 0.0;
	// */

	if ((opacitySample.x + opacitySample.y) < 127.0 / 255.0) 
		discard;

	float3 baseColor = srgbToLinear(baseColorSample.xyz);
	Surface surface = buildSurface(baseColor, normalSample.w, baseColorSample.w);

	float3 tsNormal = normalize(normalSample.xyz - 0.5);

	float3 wsNormal;
	wsNormal.x = dot(fsIn.invTransformT, tsNormal);
	wsNormal.y = dot(fsIn.invTransformB, tsNormal);
	wsNormal.z = dot(fsIn.invTransformN, tsNormal);
	wsNormal = normalize(wsNormal);

	float3 wsLight = normalize(fsIn.toLight);
	float3 wsView = normalize(fsIn.toCamera);

	BSDF bsdf = buildBSDF(wsNormal, wsLight, wsView);
	float3 directDiffuse = computeDirectDiffuse(surface, bsdf);
	float3 directSpecular = computeDirectSpecular(surface, bsdf);

	float4 brdfLookupSample = brdfLookupTexture.Sample(brdfLookupSampler, float2(surface.roughness, bsdf.VdotN));

	float3 wsDiffuseDir = diffuseDominantDirection(wsNormal, wsView, surface.roughness);
	float3 indirectDiffuse = (surface.baseColor * sampleEnvironment(wsDiffuseDir, lightDirection.xyz, 8.0)) *
		((1.0 - surface.metallness) * brdfLookupSample.z);
	                                                              
	float3 wsSpecularDir = specularDominantDirection(wsNormal, wsView, surface.roughness);
	float3 indirectSpecular = sampleEnvironment(wsSpecularDir, lightDirection.xyz, 8.0 * surface.roughness);
	indirectSpecular *= (surface.f0 * brdfLookupSample.x + surface.f90 * brdfLookupSample.y);

	float3 result = lightColor * (directDiffuse + directSpecular) + (indirectDiffuse + indirectSpecular); 

	/*
	float3 originPosition = positionOnPlanet + cameraPosition.xyz;
	float3 worldPosition = originPosition - 100.0 * fsIn.toCamera;
	float3 outScatter = outScattering(originPosition, worldPosition);

	float cosTheta = -dot(wsView, wsLight);
	float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, mieG));
	float3 inScatter = lightColor * inScattering(originPosition, worldPosition, wsLight, float2(phase.x, 0.0));

	result = result * outScatter + inScatter;
	// */                 
	
	return float4(result, 1.0);
}                              
