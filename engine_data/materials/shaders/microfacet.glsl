#include <et>

layout (std140, set = VariablesSetIndex, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

layout (std140, set = VariablesSetIndex, binding = MaterialVariablesBufferIndex) uniform MaterialVariables {
	vec4 albedoColor;
	vec4 reflectanceColor;
	vec4 emissiveColor;
	float roughness;
	float metallness;
} materialVariables;

layout (std140, set = VariablesSetIndex, binding = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
	mat4 worldRotationTransform;	
} objectVariables;

layout(binding = AlbedoTextureBinding, set = TexturesSetIndex) uniform sampler2D albedoTexture;
layout(binding = NormalTextureBinding, set = TexturesSetIndex) uniform sampler2D normalTexture;

struct VSOutput {
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
	mat3 rotationTransform = mat3(objectVariables.worldRotationTransform);
	vec4 transformedPosition = objectVariables.worldTransform * vec4(position, 1.0);

	vec3 tNormal = rotationTransform * normal;
	vec3 tTangent = rotationTransform * tangent;
	vec3 tBiTangent = cross(tNormal, tTangent);
	vec3 tCamera = (passVariables.cameraPosition - transformedPosition).xyz;
	vec3 tLight = (passVariables.lightPosition - transformedPosition * passVariables.lightPosition.w).xyz;
	
	vsOut.toCamera.x = dot(tTangent, tCamera);
	vsOut.toCamera.y = dot(tBiTangent, tCamera);
	vsOut.toCamera.z = dot(tNormal, tCamera);
	vsOut.toLight.x = dot(tTangent, tLight);
	vsOut.toLight.y = dot(tBiTangent, tLight);
	vsOut.toLight.z = dot(tNormal, tLight);
	vsOut.texCoord0 = texCoord0;

	gl_Position = passVariables.viewProjection * transformedPosition;
}

#elif defined(ET_FRAGMENT_SHADER)

#include "lighting.inl"

layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

void main()
{
	vec3 normal = normalize(texture(normalTexture, fsIn.texCoord0).xyz - 0.5);

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
	
	float diffuseComponent = normalizedLambert(env);
	float specularComponent = microfacetSpecular(env);

	vec4 diffuse = (albedoSample * materialVariables.albedoColor) * diffuseComponent;
	vec4 specular = materialVariables.reflectanceColor * specularComponent;

	outColor0 = materialVariables.emissiveColor + diffuse + specular;
}

#else
#
#	error Invalid or unsupported shader
#
#endif