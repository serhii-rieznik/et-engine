#include <et>

layout (std140, set = 0, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

layout (std140, set = 0, binding = MaterialVariablesBufferIndex) uniform MaterialVariables {
	vec4 albedoColor;
	vec4 reflectanceColor;
	float roughness;
	float metallness;
} materialVariables;

layout (std140, set = 0, binding = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
	mat4 worldRotationTransform;	
} objectVariables;

layout(binding = 0, set = 1) uniform sampler2D albedoTexture;

struct VSOutput {
	vec3 normal;
	vec3 toLight;
	vec3 toCamera;
	vec2 texCoord0;
};

#include <inputdefines>
#include <stagedefine>

#if defined(ET_VERTEX_SHADER)

#include <inputlayout>

layout (location = 0) out VSOutput vsOut;

void main()
{
	vec4 transformedPosition = objectVariables.worldTransform * vec4(position, 1.0);
	
	vsOut.normal = (objectVariables.worldRotationTransform * vec4(normal, 0.0)).xyz;
	vsOut.toCamera = (passVariables.cameraPosition - transformedPosition).xyz;
	vsOut.toLight = (passVariables.lightPosition - transformedPosition * passVariables.lightPosition.w).xyz;
	vsOut.texCoord0 = texCoord0;

	gl_Position = passVariables.viewProjection * transformedPosition;
}

#elif defined(ET_FRAGMENT_SHADER)

#include "lighting.inl"

layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

void main()
{
	vec3 normal = normalize(fsIn.normal);
	vec3 lightNormal = normalize(fsIn.toLight);
	vec3 viewNormal = normalize(fsIn.toCamera);
	vec3 halfVector = normalize(lightNormal + viewNormal);

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
	env.viewFresnel = fresnelShlick(env.metallness, env.VdotN);

	vec4 albedoSample = texture(albedoTexture, fsIn.texCoord0);
	
	float diffuseComponent = 0.5 + 0.5 * normalizedLambert(env);
	float specularComponent = microfacetSpecular(env);

	vec4 diffuse = (albedoSample * materialVariables.albedoColor) * (diffuseComponent * diffuseComponent);
	vec4 specular = materialVariables.reflectanceColor * specularComponent;

	outColor0 = diffuse + specular;
}

#else
#
#	error Invalid or unsupported shader
#
#endif