#include <et>
#include <inputlayout>

/*
 * Inputs / outputs
 */

struct VSOutput {
	float4 position [[position]];
	float3 normal;
	float3 toCamera;
	float3 toLight;
	float2 texCoord0;
};

struct FSOutput {
	float4 color0 [[color(0)]];
};

struct MaterialVariables {
	float4 albedoColor;
	float4 reflectanceColor;
	float roughness;
	float metallness;
};

struct ObjectVariables {
	float4x4 worldTransform;
	float4x4 worldRotationTransform;
};

/*
 * Vertex shader
 */
vertex VSOutput vertexMain(VSInput in [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	float4 transformedPosition = objectVariables.worldTransform * float4(in.position, 1.0);
	VSOutput out;
	out.position = passVariables.viewProjection * transformedPosition;
	out.normal = (objectVariables.worldRotationTransform * float4(in.normal, 0.0)).xyz;
	out.toCamera = (passVariables.cameraPosition - transformedPosition).xyz;
	out.toLight = (passVariables.lightPosition - transformedPosition * passVariables.lightPosition.w).xyz;
	out.texCoord0 = in.texCoord0;
	return out;
}

/*
 * Fragment shader
 */

#include "lighting.inl"

fragment FSOutput fragmentMain(VSOutput in [[stage_in]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	texture2d<float> albedoTexture [[texture(0)]],
	sampler albedoSampler [[sampler(0)]])
{
	float3 normal = normalize(in.normal);
	float3 lightNormal = normalize(in.toLight);
	float3 viewNormal = normalize(in.toCamera);
	float3 halfVector = normalize(lightNormal + viewNormal);

	float linearRoughness = materialVariables.roughness;
	linearRoughness *= linearRoughness;

	PBSLightEnvironment env;
	env.alpha = linearRoughness * linearRoughness;
	env.metallness = materialVariables.metallness;
	env.LdotN = dot(lightNormal, normal);
	env.VdotN = dot(viewNormal, normal);
	env.LdotH = dot(lightNormal, halfVector);
	env.VdotH = dot(viewNormal, halfVector);
	env.NdotH = dot(normal, halfVector);
	env.viewFresnel = fresnelShlick(materialVariables.metallness, env.VdotN);

	float4 albedoSample = albedoTexture.sample(albedoSampler, in.texCoord0);
	
	float4 diffuse = (albedoSample * materialVariables.albedoColor) * normalizedLambert(env);
	float4 specular = materialVariables.reflectanceColor * microfacetSpecular(env);

	FSOutput out;
	out.color0 = diffuse + specular;
	return out;
}
