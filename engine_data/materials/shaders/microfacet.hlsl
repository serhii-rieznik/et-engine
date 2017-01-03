#include <et>
#include <inputdefines>
#include <inputlayout>

cbuffer MaterialVariables : CONSTANT_LOCATION(b, MaterialVariablesBufferIndex, VariablesSetIndex)
{
	float4 baseColorScale;
	float4 emissiveColor;
	float roughnessScale;
	float metallnessScale;
};

cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex)
{
	float4x4 worldTransform;
	float4x4 worldRotationTransform;	
};

Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
SamplerState baseColorSampler;

Texture2D<float4> normalRoughnessMetallnessTexture : CONSTANT_LOCATION(t, NormalRoughnesMetallnessTextureBinding, TexturesSetIndex);
SamplerState normalRoughnessMetallnessSampler;

Texture2D<float4> shadowTexture : CONSTANT_LOCATION(t, ShadowTextureBinding, SharedTexturesSetIndex);
SamplerState shadowSampler;

Texture2D<float4> environmentTexture : CONSTANT_LOCATION(t, EnvironmentTextureBinding, SharedTexturesSetIndex);
SamplerState environmentSampler;

struct VSOutput 
{
	float4 position : SV_Position;
	float3 viewWorldSpace : POSITION0;
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
	float3 tNormal = mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz;
	float3 tCamera = (cameraPosition - transformedPosition).xyz;
	float3 tTangent = mul(float4(vsIn.tangent, 0.0), worldRotationTransform).xyz;
	float3 tBiTangent = cross(tNormal, tTangent);
	float3 tLight = (lightPosition - transformedPosition * lightPosition.w).xyz;

	VSOutput vsOut;
	vsOut.viewWorldSpace = tCamera;
	vsOut.invTransformT = float3(tTangent.x, tBiTangent.x, tNormal.x);
	vsOut.invTransformB = float3(tTangent.y, tBiTangent.y, tNormal.y);
	vsOut.invTransformN = float3(tTangent.z, tBiTangent.z, tNormal.z);
	vsOut.toCamera.x = dot(tTangent, tCamera);
	vsOut.toCamera.y = dot(tBiTangent, tCamera);
	vsOut.toCamera.z = dot(tNormal, tCamera);
	vsOut.toLight.x = dot(tTangent, tLight);
	vsOut.toLight.y = dot(tBiTangent, tLight);
	vsOut.toLight.z = dot(tNormal, tLight);
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.lightCoord = mul(transformedPosition, lightProjection);
	vsOut.position = mul(transformedPosition, viewProjection);
	return vsOut;
}

#include "lighting.inl"

float sampleShadow(float3 tc)
{
	float shadowSample = 0.5 + 0.5 * shadowTexture.Sample(shadowSampler, tc.xy).x;
	return float(tc.z < shadowSample);
}

float4 sampleEnvironment(float3 i, float lod)
{
	float2 sampleCoord = float2((atan2(i.z, i.x) + PI) / DOUBLE_PI, (asin(i.y) + HALF_PI) / PI);
	return environmentTexture.SampleLevel(environmentSampler, sampleCoord, lod);
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float3 normal = normalize(normalRoughnessMetallnessTexture.Sample(normalRoughnessMetallnessSampler, fsIn.texCoord0).xyz + float3(-0.5, -0.5, 0.0));

	float3 wsNormal;
	wsNormal.x = dot(fsIn.invTransformT, normal);
	wsNormal.y = dot(fsIn.invTransformB, normal);
	wsNormal.z = dot(fsIn.invTransformN, normal);
	wsNormal = normalize(wsNormal);

	float3 lightNormal = normalize(fsIn.toLight);
	float3 viewNormal = normalize(fsIn.toCamera);
	float3 halfVector = normalize(lightNormal + viewNormal);

	float linearRoughness = roughnessScale;
	linearRoughness *= linearRoughness;

	PBSLightEnvironment env;
	env.alpha = linearRoughness * linearRoughness;
	env.metallness = metallnessScale;
	env.LdotN = dot(lightNormal, normal);
	env.VdotN = dot(viewNormal, normal);
	env.LdotH = dot(lightNormal, halfVector);
	env.VdotH = dot(viewNormal, halfVector);
	env.NdotH = dot(normal, halfVector);
	env.viewFresnel = fresnelShlick(env.metallness, env.VdotN);

	float hackOcclusion = 0.5 + 0.5 * wsNormal.y;
	hackOcclusion *= hackOcclusion;

	float diffuseComponent = DOUBLE_PI * normalizedLambert(env);
	float specularComponent = microfacetSpecular(env);
	float shadowSample = 0.1 + 0.9 * sampleShadow(fsIn.lightCoord.xyz / fsIn.lightCoord.w);

	float4 albedoSample = baseColorScale * sampleEnvironment(wsNormal, 10.0) * baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0);
	float4 specularSample = baseColorScale * sampleEnvironment(-reflect(normalize(fsIn.viewWorldSpace), wsNormal), 10.0 * roughnessScale);
	
	return lerp(albedoSample, specularSample, env.viewFresnel) * (specularComponent + hackOcclusion * diffuseComponent * shadowSample);
}
