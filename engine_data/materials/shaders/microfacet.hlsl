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
	row_major float4x4 worldTransform;
	row_major float4x4 worldRotationTransform;	
};

Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
SamplerState baseColorSampler : CONSTANT_LOCATION(s, BaseColorSamplerBinding, TexturesSetIndex);

Texture2D<float4> normalTexture : CONSTANT_LOCATION(t, NormalTextureBinding, TexturesSetIndex);
SamplerState normalSampler : CONSTANT_LOCATION(s, NormalSamplerBinding, TexturesSetIndex);;

Texture2D<float4> roughnessTexture : CONSTANT_LOCATION(t, RoughnessTextureBinding, TexturesSetIndex);
SamplerState roughnessSampler : CONSTANT_LOCATION(s, RoughnessSamplerBinding, TexturesSetIndex);;

Texture2D<float4> metallnessTexture : CONSTANT_LOCATION(t, MetallnessTextureBinding, TexturesSetIndex);
SamplerState metallnessSampler : CONSTANT_LOCATION(s, MetallnessSamplerBinding, TexturesSetIndex);;

Texture2D<float4> shadowTexture : CONSTANT_LOCATION(t, ShadowTextureBinding, SharedTexturesSetIndex);
SamplerState shadowSampler : CONSTANT_LOCATION(s, ShadowSamplerBinding, SharedTexturesSetIndex);;

Texture2D<float4> environmentTexture : CONSTANT_LOCATION(t, EnvironmentTextureBinding, SharedTexturesSetIndex);
SamplerState environmentSampler : CONSTANT_LOCATION(s, EnvironmentSamplerBinding, SharedTexturesSetIndex);

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
	float4 transformedPosition = 
		// float4(vsIn.position, 1.0); 
		mul(float4(vsIn.position, 1.0), worldTransform);

	float3 tNormal = mul(float4(vsIn.normal, 0.0), worldRotationTransform).xyz;
	float3 tTangent = mul(float4(vsIn.tangent, 0.0), worldRotationTransform).xyz;
	float3 tBiTangent = cross(tNormal, tTangent);

	VSOutput vsOut;
	vsOut.toCamera = (cameraPosition - transformedPosition).xyz;
	vsOut.toLight = (lightPosition - transformedPosition * lightPosition.w).xyz;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.lightCoord = mul(transformedPosition, lightProjection);
	vsOut.position = mul(transformedPosition, viewProjection);
	vsOut.invTransformT = float3(tTangent.x, tBiTangent.x, tNormal.x);
	vsOut.invTransformB = float3(tTangent.y, tBiTangent.y, tNormal.y);
	vsOut.invTransformN = float3(tTangent.z, tBiTangent.z, tNormal.z);
	return vsOut;
}

#include "lighting.inl"

float sampleShadow(float3 tc)
{
	float shadowSample = 0.5 + 0.5 * shadowTexture.Sample(shadowSampler, tc.xy).x;
	return float(tc.z <= shadowSample);
}

float4 sampleEnvironment(float3 i, float lod)
{
	float2 sampleCoord = float2((atan2(i.z, i.x) + PI) / DOUBLE_PI, (asin(i.y) + HALF_PI) / PI);
	return environmentTexture.SampleLevel(environmentSampler, sampleCoord, lod);
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
	float3 tsNormal = float3(0.0, 0.0, 1.0); // normalize(normalTexture.Sample(normalSampler, fsIn.texCoord0).xyz + float3(-0.5, -0.5, 0.0));

	float3 wsNormal;
	wsNormal.x = dot(fsIn.invTransformT, tsNormal);
	wsNormal.y = dot(fsIn.invTransformB, tsNormal);
	wsNormal.z = dot(fsIn.invTransformN, tsNormal);
	wsNormal = normalize(wsNormal);

	float3 lightNormal = normalize(fsIn.toLight);
	float3 viewNormal = normalize(fsIn.toCamera);
	float3 halfVector = normalize(lightNormal + viewNormal);

	float roughness = clamp(roughnessScale, 0.01, 1.0);
	float metallness = saturate(metallnessScale);

	PBSLightEnvironment env;
	env.alpha = roughness * roughness;
	env.metallness = metallnessScale;
	env.LdotN = dot(lightNormal, wsNormal);
	env.VdotN = dot(viewNormal, wsNormal);
	env.LdotH = dot(lightNormal, halfVector);
	env.VdotH = dot(viewNormal, halfVector);
	env.NdotH = dot(wsNormal, halfVector);
	env.viewFresnel = fresnelShlick(env.metallness, env.VdotN);
	env.brdfFresnel = fresnelShlick(env.metallness, env.LdotH);

	float shadowSample = sampleShadow(fsIn.lightCoord.xyz / fsIn.lightCoord.w);

	float3 ambientColor = sampleEnvironment(wsNormal, 10.0).xyz;

	float3 directDiffuse = baseColorTexture.Sample(baseColorSampler, fsIn.texCoord0).xyz;
	float directDiffuseTerm = shadowSample * normalizedLambert(env);

	float3 directSpecular = microfacetSpecular(env);
	float3 reflection = env.viewFresnel * sampleEnvironment(reflect(normalize(fsIn.toCamera), wsNormal), 10.0 * roughness).xyz;

	float3 diffuse = baseColorScale.xyz * (directDiffuse * directDiffuseTerm + ambientColor) * (1.0 - metallness);
	float3 specular = reflection + directSpecular * shadowSample;

	return 
	// metallness;
	// shadowSample;
	// env.LdotN;
	// float4(fsIn.toLight, 1.0);
	float4(diffuse + specular, 1.0);
}
