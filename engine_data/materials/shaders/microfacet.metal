#include "include/common.metal.inl"
#include "include/lighting.inl"

/*
 * Inputs / outputs
 */

%built-in-input%

struct VSOutput {
	float4 position [[position]];
	float3 normal;
	float3 toCamera;
	float3 toLight;
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
	return out;
}

/*
 * Fragment shader
 */

fragment FSOutput fragmentMain(VSOutput in [[stage_in]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]])
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

	FSOutput out;

	out.color0 =
	/*
		materialVariables.albedoColor * burleyDiffuse(env) +
		materialVariables.reflectanceColor * microfacetSpecular(env);
	// */
	float4(burleyDiffuse(env));
	// float4(microfacetSpecular(env));

	return out;
}
