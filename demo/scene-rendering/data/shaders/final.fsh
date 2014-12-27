uniform sampler2D texture_depth;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_occlusion;
uniform sampler2D texture_noise;

uniform mat4 mModelViewInverseToPrevious;

uniform vec3 lightPositions[10];
uniform int lightsCount;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NoiseTexCoord;
etFragmentIn vec2 NormalizedTexCoord;

const vec4 luma = vec4(0.299, 0.587, 0.114, 0.0);
const vec4 ambientColor = vec4(0.15, 0.175, 0.225, 0.0);

#include "include/viewspace.fsh"
#include "include/normals.fsh"

float computeFresnelTerm(in float VdotN, in float indexOfRefraction)
{
	float eta = indexOfRefraction * VdotN;
	float eta2 = eta * eta;
	float beta = 1.0 - indexOfRefraction * indexOfRefraction;
	float result = 1.0 + 2.0 * (eta2 - eta * sqrt(beta + eta2)) / beta;
	return clamp(result * result, 0.0, 1.0);
}

#define REFLECTION_SAMPLES	25

void main()
{
	vec4 diffuseSample = etTexture2D(texture_diffuse, TexCoord);
	vec4 occlusionSample = etTexture2D(texture_occlusion, TexCoord);
	
	float depthSample = 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0;
	vec3 viewSpacePosition = restoreViewSpacePosition(NormalizedTexCoord, depthSample);
	vec3 normalSample = decodeNormal(etTexture2D(texture_normal, TexCoord).xy);
	
	vec3 vView = normalize(viewSpacePosition);
	
	vec3 reflectedVector = reflect(vView, normalSample);
	
	float VdotN = max(0.0, -dot(normalSample, vView));
	float fresnel = computeFresnelTerm(VdotN, 1.0 / 1.32);
	
	vec3 reflectionPoint = vec3(0.0);
	vec4 reflectionSample = vec4(0.0);
	float reflectionDistance = -viewSpacePosition.z;

	vec4 noiseSample = etTexture2D(texture_noise, NoiseTexCoord);
	
	for (int s = 0; s < REFLECTION_SAMPLES; ++s)
	{
		vec3 randomDirection = randomVectorOnHemisphereWithScale(reflectedVector, noiseSample.xyz, 8.5);
		
		for (int i = 0; i < 3; ++i)
		{
			reflectionPoint = viewSpacePosition + randomDirection * reflectionDistance;
			vec3 projected = projectViewSpacePosition(reflectionPoint);
			float restoredDepth = restoreViewSpaceDistance(2.0 * etTexture2D(texture_depth, projected.xy).x - 1.0);
			reflectionDistance = max(0.0, viewSpacePosition.z - restoredDepth);
		}
		
		reflectionPoint = viewSpacePosition + randomDirection * reflectionDistance;
		vec3 finalProjecttion = projectViewSpacePosition(reflectionPoint);
		
		reflectionSample += etTexture2D(texture_diffuse, finalProjecttion.xy);
		noiseSample = etTexture2D(texture_noise, NoiseTexCoord + noiseSample.yz);
	}
	
	diffuseSample = mix(diffuseSample, reflectionSample / float(REFLECTION_SAMPLES), fresnel);

	float lighting = 0.0;

	for (int i = 0; i < lightsCount; ++i)
	{
		vec3 lightDirection = lightPositions[i] - viewSpacePosition.xyz;
		float lightDirectionLengthSquare = dot(lightDirection, lightDirection);
		float LdotN = max(0.0, dot(normalSample, lightDirection * inversesqrt(lightDirectionLengthSquare)));
		float attenuation = LdotN / (1.0 + 0.000025 * lightDirectionLengthSquare);
		lighting += attenuation;
	}

	float occlusion = pow(1.0 - occlusionSample.w, 4.0);
	vec4 lightBounce = mix(vec4(dot(occlusionSample, luma)), occlusionSample, 2.0);
	
	etFragmentOut = (ambientColor * occlusion + lighting) * diffuseSample * occlusion + lightBounce * lighting;
}
