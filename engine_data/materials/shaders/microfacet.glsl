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
layout(binding = ShadowTextureBinding, set = SharedTexturesSetIndex) uniform sampler2D shadowTexture;
layout(binding = EnvironmentTextureBinding, set = SharedTexturesSetIndex) uniform sampler2D environmentTexture;

struct VSOutput {
	vec3 viewWorldSpace;
	vec3 toLight;
	vec3 toCamera;
	vec2 texCoord0;
	vec4 lightCoord;
	vec3 invTransformT;
	vec3 invTransformB;
	vec3 invTransformN;
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
	vec3 tCamera = (passVariables.cameraPosition - transformedPosition).xyz;

	vec3 tTangent = rotationTransform * tangent;
	vec3 tBiTangent = cross(tNormal, tTangent);
	vec3 tLight = (passVariables.lightPosition - transformedPosition * passVariables.lightPosition.w).xyz;

	vsOut.viewWorldSpace = tCamera;
	vsOut.invTransformT = vec3(tTangent.x, tBiTangent.x, tNormal.x);
	vsOut.invTransformB = vec3(tTangent.y, tBiTangent.y, tNormal.y);
	vsOut.invTransformN = vec3(tTangent.z, tBiTangent.z, tNormal.z);

	vsOut.toCamera.x = dot(tTangent, tCamera);
	vsOut.toCamera.y = dot(tBiTangent, tCamera);
	vsOut.toCamera.z = dot(tNormal, tCamera);

	vsOut.toLight.x = dot(tTangent, tLight);
	vsOut.toLight.y = dot(tBiTangent, tLight);
	vsOut.toLight.z = dot(tNormal, tLight);

	vsOut.texCoord0 = texCoord0;
	vsOut.lightCoord = passVariables.lightProjection * transformedPosition;

	gl_Position = passVariables.viewProjection * transformedPosition;
}

#elif defined(ET_FRAGMENT_SHADER)

#include "lighting.inl"

layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

float sampleShadow(vec3 tc)
{
	float shadowSample = 0.5 + 0.5 * texture(shadowTexture, tc.xy).x;
	return float(tc.z < shadowSample);
}

vec4 sampleEnvironment(vec3 i, float lod)
{
	vec2 sampleCoord = vec2((atan(i.z, i.x) + PI) / DOUBLE_PI, (asin(i.y) + HALF_PI) / PI);
	return textureLod(environmentTexture, sampleCoord, lod);
}

void main()
{
	vec3 normal = normalize(texture(normalTexture, fsIn.texCoord0).xyz + vec3(-0.5, -0.5, 0.0));

	vec3 wsNormal;
	wsNormal.x = dot(fsIn.invTransformT, normal);
	wsNormal.y = dot(fsIn.invTransformB, normal);
	wsNormal.z = dot(fsIn.invTransformN, normal);
	wsNormal = normalize(wsNormal);

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

	float hackOcclusion = 0.5 + 0.5 * wsNormal.y;
	hackOcclusion *= hackOcclusion;

	float diffuseComponent = DOUBLE_PI * normalizedLambert(env);
	float specularComponent = microfacetSpecular(env);
	float shadowSample = 0.1 + 0.9 * sampleShadow(fsIn.lightCoord.xyz / fsIn.lightCoord.w);

	vec4 albedoSample = materialVariables.albedoColor * 
		sampleEnvironment(wsNormal, 10.0) * texture(albedoTexture, fsIn.texCoord0);
	
	vec4 specularSample = materialVariables.reflectanceColor * 
		sampleEnvironment(-reflect(normalize(fsIn.viewWorldSpace), wsNormal), 10.0 * materialVariables.roughness);
	
	outColor0 = vec4(specularComponent + hackOcclusion * diffuseComponent * shadowSample) * mix(albedoSample, specularSample, env.viewFresnel);
}

#else
#
#	error Invalid or unsupported shader
#
#endif