#include <et>
#include <inputdefines>
#include <inputlayout>
#include <options>
#include "common.h"

#define EnableClearCoat 0
#define EnableIridescence 0

cbuffer MaterialVariables : DECL_MATERIAL_BUFFER
{
    float4 diffuseReflectance;
    float4 specularReflectance;
    float4 emissiveColor;
    float roughnessScale;
    float metallnessScale;
};

cbuffer ObjectVariables : DECL_OBJECT_BUFFER
{
    row_major float4x4 viewProjectionTransform;
    row_major float4x4 previousViewProjectionTransform;
    
    row_major float4x4 worldTransform;
    row_major float4x4 previousWorldTransform;

    row_major float4x4 worldRotationTransform;
    row_major float4x4 lightViewTransform;
    row_major float4x4 lightProjectionTransform;
	float4 environmentSphericalHarmonics[9];
	float4 cameraJitter;
    float4 cameraPosition;
    float4 lightDirection;
    float3 lightColor;
    float4 viewport;
	float continuousTime;
};

Texture2D<float4> baseColor : DECLARE_TEXTURE;
Texture2D<float4> normal : DECLARE_TEXTURE;
Texture2D<float4> brdfLookup : DECLARE_TEXTURE;
Texture2D<float4> opacity : DECLARE_TEXTURE;
Texture2D<float4> ltcTransform : DECLARE_TEXTURE;
Texture2D<float> noise : DECLARE_TEXTURE;
Texture2D<float> ao : DECLARE_TEXTURE;

struct VSOutput 
{
    float4 position : SV_Position;
   	float4 projectedPosition;
   	float4 previousProjectedPosition;
    float3 normal;
    float3 toLight;
    float3 toCamera;
    float2 texCoord0;
    float4 lightCoord;
    float3 invTransformT;
    float3 invTransformB;
    float3 invTransformN;
    float3 worldPosition;
};

#if (EnableClearCoat)
static const float ClearCoatEta = 1.76;
static const float ClearCoatRoughness = 0.05;
#endif

VSOutput vertexMain(VSInput vsIn)
{
    float4 transformedPosition = mul(float4(vsIn.position, 1.0), worldTransform);
    float4 previousTransformedPosition = mul(float4(vsIn.position, 1.0), previousWorldTransform);

    VSOutput vsOut;
    vsOut.texCoord0 = vsIn.texCoord0;
    vsOut.normal = normalize(mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz);

    float3 tTangent = normalize(mul(float4(vsIn.tangent, 0.0), worldRotationTransform).xyz);
    float3 tBiTangent = cross(vsOut.normal, tTangent);

    vsOut.worldPosition =transformedPosition.xyz;
    vsOut.toCamera = (cameraPosition.xyz - transformedPosition.xyz).xyz;
    vsOut.toLight = (lightDirection.xyz - transformedPosition.xyz * lightDirection.w).xyz;
    vsOut.lightCoord = mul(mul(transformedPosition, lightViewTransform), lightProjectionTransform);
    vsOut.invTransformT = float3(tTangent.x, tBiTangent.x, vsOut.normal.x);
    vsOut.invTransformB = float3(tTangent.y, tBiTangent.y, vsOut.normal.y);
    vsOut.invTransformN = float3(tTangent.z, tBiTangent.z, vsOut.normal.z);
    vsOut.projectedPosition = mul(transformedPosition, viewProjectionTransform);
    vsOut.previousProjectedPosition = mul(previousTransformedPosition, previousViewProjectionTransform);
    vsOut.position = vsOut.projectedPosition;

    return vsOut;
}

#include "srgb.h"
#include "bsdf.h"
#include "environment.h"
#include "atmosphere.h"
#include "importance-sampling.h"
#include "moments.h"
#include "shadowmapping.h"
#include "iridescence.h"

struct FSOutput 
{
	float4 color : SV_Target0;
	float2 velocity : SV_Target1;
};

float integrateEdge(in float3 p1, in float3 p2)
{
	float cosTheta = dot(p1, p2);
	float theta = acos(cosTheta);
	return cross(p1, p2).z * ((theta > 0.001) ? theta / sin(theta) : 0.0);
}

float3 evaluateLTC(in float3 n, in float3 v, in float3 p, in float3x3 inverseTransform, in float3 points[4])
{
	float3 t1 = normalize(v - n * dot(n, v));
	float3 t2 = cross(n, t1);
	row_major float3x3 t = inverseTransform * float3x3(t1.x, t2.x, n.x, t1.y, t2.y, n.y, t1.z, t2.z, n.z);
	
	points[0] -= p;
	points[1] -= p;
	points[2] -= p;
	points[3] -= p;

	points[0] = mul(points[0], t);
	points[1] = mul(points[1], t);
	points[2] = mul(points[2], t);
	points[3] = mul(points[3], t);

	points[0] = normalize(points[0]);
	points[1] = normalize(points[1]);
	points[2] = normalize(points[2]);
	points[3] = normalize(points[3]);
	
	float result = 0.0;
	result += integrateEdge(points[0], points[1]);
	result += integrateEdge(points[1], points[2]);
	result += integrateEdge(points[2], points[3]);
	result += integrateEdge(points[3], points[0]);
	return max(0.0, result);
}

FSOutput fragmentMain(VSOutput fsIn)
{
    float4 opacitySample = opacity.Sample(AnisotropicWrap, fsIn.texCoord0);
    if (opacitySample.x < ALPHA_TEST_TRESHOLD) 
    	discard;

    float4 baseColorSample = baseColor.Sample(AnisotropicWrap, fsIn.texCoord0);
    float4 normalSample = normal.Sample(AnisotropicWrap, fsIn.texCoord0);

	float3 noiseDimensions = 0.0;
	noise.GetDimensions(0, noiseDimensions.x, noiseDimensions.y, noiseDimensions.z);

	float3 shadowmapSize = 0.0;
	shadow.GetDimensions(0, shadowmapSize.x, shadowmapSize.y, shadowmapSize.z);

    float2 currentPosition = fsIn.projectedPosition.xy / fsIn.projectedPosition.w;
    float2 previousPosition = fsIn.previousProjectedPosition.xy / fsIn.previousProjectedPosition.w;

    float2 projectedUv = currentPosition.xy * 0.5 + 0.5;
	float2 noiseUV = (2.0 * projectedUv * viewport.zw / noiseDimensions.xy);
	float sampledNoise = noise.Sample(PointClamp, 0.5 * noiseUV);

    float2 nxy = normalSample.xy * 2.0 - 1.0;
    float3 tsNormal = normalize(float3(nxy, sqrt(1.0 - saturate(dot(nxy, nxy)))));
    float3 wsNormal = normalize(float3(dot(fsIn.invTransformT, tsNormal), dot(fsIn.invTransformB, tsNormal), dot(fsIn.invTransformN, tsNormal)));

    float3 baseColor = srgbToLinear(diffuseReflectance.xyz * baseColorSample.xyz);
    float roughness = normalSample.z;
    float metallness = normalSample.w;
    float ambientOcclusion = ao.Sample(LinearClamp, projectedUv).x;
	float shadowValue = sampleShadow(fsIn.lightCoord.xyz / fsIn.lightCoord.w, sampledNoise, shadowmapSize.xy);
        
    Surface surface = buildSurface(baseColor, roughness, metallness);

    float3 wsLight = normalize(fsIn.toLight);
    float3 wsView = normalize(fsIn.toCamera);

    BSDF bsdf = buildBSDF(wsNormal, wsLight, wsView);
    float3 directDiffuse = computeDirectDiffuse(surface, bsdf);
    float3 directSpecular = computeDirectSpecular(surface, bsdf);

    float4 brdfLookupSample = brdfLookup.Sample(LinearClamp, float2(surface.roughness, bsdf.VdotN));

    float3 wsDiffuseDir = diffuseDominantDirection(wsNormal, wsView, surface.roughness);
    float3 indirectDiffuseSample = getExitRadianceFromSphericalHarmonics(environmentSphericalHarmonics, wsDiffuseDir).xyz;
    float3 indirectDiffuse = (surface.baseColor * indirectDiffuseSample) * ((1.0 - surface.metallness) * brdfLookupSample.z);
                                                                  
    float3 wsSpecularDir = specularDominantDirection(wsNormal, wsView, surface.roughness);
    float3 indirectSpecularSample = sampleSpecularConvolution(wsSpecularDir, surface.roughness);
    float3 indirectSpecular = indirectSpecularSample * (surface.f0 * brdfLookupSample.x + surface.f90 * brdfLookupSample.y);

    float3 emitter[4];
    {
		emitter[3] = float3(-15.0,  5.0, -50.0);
		emitter[2] = float3( 15.0,  5.0, -50.0);
		emitter[1] = float3( 15.0, 35.0, -50.0);
		emitter[0] = float3(-15.0, 35.0, -50.0);
    };
    float2 ltcSampleCoords;
	ltcSampleCoords.x =	pow(1.0 + surface.roughness, 3.0) / 8.0;
	ltcSampleCoords.y = acos(bsdf.VdotN) / (0.5 * PI);

	float4 ltcSample = ltcTransform.Sample(LinearClamp, ltcSampleCoords);

    float3x3 ltcTransform = float3x3(
    	1.0, 0.0, ltcSample.y,
    	0.0, ltcSample.z, 0.0,
    	ltcSample.w, 0.0, ltcSample.x);

    float3x3 identityTransform = float3x3(
    	1.0, 0.0, 0.0,
    	0.0, 1.0, 0.0,
    	0.0, 0.0, 1.0);

    float3 ltcColor = float3(50000.0, 25000.0, 0.0);
    float3 ltcSpecular = evaluateLTC(wsNormal, wsView, fsIn.worldPosition, ltcTransform, emitter);
    float3 ltcDiffuse = evaluateLTC(wsNormal, wsView, fsIn.worldPosition, identityTransform, emitter) * (1.0 - surface.metallness);
        
#if (EnableIridescence)
    BSDF iblBsdf = buildBSDF(wsNormal, wsSpecularDir, wsView);
    float3 fresnelScale = iridescentFresnel(iblBsdf);
    indirectSpecular *= fresnelScale / max(fresnelScale.x, max(fresnelScale.y, fresnelScale.z));
#endif

#if (EnableClearCoat)
    Surface ccSurface = buildSurface(float3(1.0, 1.0, 1.0), 0.0, ClearCoatRoughness);
    BSDF ccBSDF = buildBSDF(fsIn.normal, wsLight, wsView);
	float3 ccSpecular = computeDirectSpecular(ccSurface, ccBSDF);

    float4 ccBrdfLookupSample = brdfLookup.Sample(LinearClamp, float2(ccSurface.roughness, ccBSDF.VdotN));
                                                                  
    wsSpecularDir = specularDominantDirection(fsIn.normal, wsView, ccSurface.roughness);
    float3 ccIndirectSpecular = sampleSpecularConvolution(wsSpecularDir, ccSurface.roughness);
    ccIndirectSpecular *= (ccSurface.f0 * ccBrdfLookupSample.x + ccSurface.f90 * ccBrdfLookupSample.y);

    float f0 = (ClearCoatEta - 1.0) / (ClearCoatEta + 1.0);
    float clearCoatFresnel = fresnel(f0 * f0, 1.0, ccBSDF.VdotN);

    float3 result = 
    	lerp(directDiffuse + directSpecular, ccSpecular, clearCoatFresnel) * shadowValue * lightColor + 
    	lerp(indirectDiffuse + indirectSpecular, ccIndirectSpecular, clearCoatFresnel);

#else

    float3 result = shadowValue * ((directDiffuse + directSpecular) * lightColor) + 
    	ambientOcclusion * (indirectDiffuse + indirectSpecular) + 
    	0.0 * ltcColor * (ltcDiffuse + ltcSpecular);

#endif
	
	currentPosition += cameraJitter.xy;
	previousPosition += cameraJitter.zw;

    FSOutput output;
    output.color = float4(result, 1.0);
	output.velocity = 0.5 * (previousPosition - currentPosition);
    return output;
}                              
