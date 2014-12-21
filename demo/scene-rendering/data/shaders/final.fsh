uniform sampler2D texture_depth;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_occlusion;

uniform vec3 lightPositions[10];
uniform int lightsCount;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NormalizedTexCoord;

const vec4 luma = vec4(0.299, 0.587, 0.114, 0.0);
const vec4 ambientColor = vec4(0.15, 0.175, 0.225, 0.0);

#include "viewspace.fsh"

void main()
{
	float depthSample = 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0;
	
	vec4 diffuseSample = etTexture2D(texture_diffuse, TexCoord);
	vec4 occlusionSample = etTexture2D(texture_occlusion, TexCoord);
	vec3 normalSample = normalize(normalBias * etTexture2D(texture_normal, TexCoord).xyz - normalScale);
	
	vec3 viewSpacePosition = restoreViewSpacePosition2(NormalizedTexCoord, depthSample);
	
	float lighting = 1.0;
/*
	for (int i = 0; i < lightsCount; ++i)
	{
		vec3 lightDirection = lightPositions[i] - viewSpacePosition;
		float attenuation = 15.0 / (1.0 + 0.002 * dot(lightDirection, lightDirection));
		lighting += dot(normalSample, normalize(lightDirection));
	}
*/
	float occlusion = pow(1.0 - occlusionSample.w, 4.0);
	vec4 lightBounce = mix(vec4(dot(occlusionSample, luma)), occlusionSample, 2.0);

	etFragmentOut =
	//lightBounce;
	//vec4(occlusion);
	//occlusionSample;
	//vec4(length(lightPositions[0] - viewSpacePosition) / 500.0);
	//(ambientColor * occlusion + diffuseSample * lighting) * occlusion + lightBounce * lighting;
	occlusion * diffuseSample * mix(occlusion * ambientColor, vec4(1.0), lighting) + lightBounce * lighting;
}
