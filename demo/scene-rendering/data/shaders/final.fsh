uniform sampler2D texture_depth;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_occlusion;

uniform mat4 mModelViewInverseToPrevious;

uniform vec3 lightPositions[10];
uniform int lightsCount;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NormalizedTexCoord;

const vec4 luma = vec4(0.299, 0.587, 0.114, 0.0);
const vec4 ambientColor = vec4(0.15, 0.175, 0.225, 0.0);

#include "include/viewspace.fsh"
#include "include/normals.fsh"

void main()
{
	float depthSample = 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0;
	
	vec4 diffuseSample = etTexture2D(texture_diffuse, TexCoord);
	vec4 occlusionSample = etTexture2D(texture_occlusion, TexCoord);
	vec3 normalSample = decodeNormal(etTexture2D(texture_normal, TexCoord).xy);

	vec3 viewSpacePosition = restoreViewSpacePosition(NormalizedTexCoord, depthSample);

/*
	vec4 previousProjection = mModelViewInverseToPrevious * vec4(viewSpacePosition, 1.0);
	previousProjection /= previousProjection.w;
	vec2 motion = 100.0 * abs((previousProjection.xy - NormalizedTexCoord) / viewSpacePosition.z);
	etFragmentOut = vec4(motion, 0.0, 1.0);
*/
	
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
