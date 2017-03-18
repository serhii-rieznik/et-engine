#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer MaterialVariables : DECL_BUFFFER(Material)
{
	float4 diffuseReflectance;
	float4 specularReflectance;
	float4 emissiveColor;
	float roughnessScale;
	float metallnessScale;
};

cbuffer ObjectVariables : DECL_BUFFFER(Object)
{
	row_major float4x4 lightProjectionTransform;
	row_major float4x4 viewProjectionTransform;
	row_major float4x4 worldTransform;
	row_major float4x4 worldRotationTransform;
	float4 cameraPosition;
	float4 lightPosition;
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
	float3 toLight : POSITION1;
	float3 toCamera : POSITION2;
	float2 texCoord0 : TEXCOORD0;
	float4 lightCoord : TEXCOORD1;
	float3 invTransformT : TANGENT;
	float3 invTransformB : BINORMAL;
	float3 invTransformN : NORMAL;
};

VSOutput vertexMain(VSInput vsIn)
{
	float4 transformedPosition = mul(float4(vsIn.position, 1.0), worldTransform);

	float3 tNormal = normalize(mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz);
	float3 tTangent = normalize(mul(float4(vsIn.tangent, 0.0), worldRotationTransform).xyz);
	float3 tBiTangent = cross(tNormal, tTangent);

	VSOutput vsOut;
	vsOut.toCamera = (cameraPosition - transformedPosition).xyz;
	vsOut.toLight = (lightPosition - transformedPosition * lightPosition.w).xyz;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.lightCoord = mul(transformedPosition, lightProjectionTransform);
	vsOut.position = mul(transformedPosition, viewProjectionTransform);
	vsOut.invTransformT = float3(tTangent.x, tBiTangent.x, tNormal.x);
	vsOut.invTransformB = float3(tTangent.y, tBiTangent.y, tNormal.y);
	vsOut.invTransformN = float3(tTangent.z, tBiTangent.z, tNormal.z);
	return vsOut;
}

#include "srgb.h"
#include "bsdf.h"
#include "environment.h"
#include "importance-sampling.h"

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float4 normalSample = normalTexture.Sample(normalSampler, fsIn.texCoord0);
	float4 baseColorSample = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
	float4 opacitySample = opacityTexture.Sample(opacitySampler, fsIn.texCoord0);

	if ((opacitySample.x + opacitySample.y) < 127.0 / 255.0)
		discard;

	float3 baseColor = srgbToLinear(baseColorSample.xyz);
	Surface surface = buildSurface(baseColor, normalSample.w * metallnessScale, baseColorSample.w * roughnessScale);

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
	float3 indirectDiffuse = (surface.baseColor * sampleEnvironment(wsDiffuseDir, 8.0)) *
		 ((1.0 - surface.metallness) * brdfLookupSample.z);
	                                                              
	float3 wsSpecularDir = specularDominantDirection(wsNormal, wsView, surface.roughness);
	float3 indirectSpecular = sampleEnvironment(wsSpecularDir, 8.0 * surface.roughness);
	indirectSpecular *= (surface.f0 * brdfLookupSample.x + surface.f90 * brdfLookupSample.y);

	float3 result = 
			1.0 * (directDiffuse + directSpecular) + 
			1.0 * (indirectDiffuse + indirectSpecular); 

//	result = directSpecular;
//	result = indirectSpecular;
	
	return float4(linearToSRGB(result), 1.0);
}                              
