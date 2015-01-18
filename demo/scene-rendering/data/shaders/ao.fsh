uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;
uniform sampler2D texture_noise;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NoiseTexCoord;

#define	NUM_SAMPLES					24
#define MIN_SAMPLE_SIZE				0.05
#define SAMPLE_SIZE					1.0
#define DEPTH_DIFFERENCE_SCALE		5.0

#define COMPUTE_LIGHT_BOUNCE		0

#include "include/viewspace.fsh"
#include "include/normals.fsh"

#if (COMPUTE_LIGHT_BOUNCE)
	vec4 performRaytracingInViewSpace(in vec3 vp, in vec3 vn, in vec4 noise)
#else
	float performRaytracingInViewSpace(in vec3 vp, in vec3 vn, in vec4 noise)
#endif
{
	vec3 randomNormal = randomVectorOnHemisphere(vn, noise.xyz);
	vec3 projected = projectViewSpacePosition(vp + randomNormal * (MIN_SAMPLE_SIZE + noise.w * SAMPLE_SIZE));
	float sampledDepth = etTexture2D(texture_depth, projected.xy).x;
	
	if (sampledDepth > projected.z)
	{
#	if (COMPUTE_LIGHT_BOUNCE)
		return vec4(0.0);
#	else
		return 0.0;
#	endif
	}
	
	float depthDifference = DEPTH_DIFFERENCE_SCALE * (inversesqrt(1.0 - projected.z) - inversesqrt(1.0 - sampledDepth));
	float occlusion = dot(vn, randomNormal) * (1.0 - noise.w) / (1.0 + depthDifference * depthDifference);

#if (COMPUTE_LIGHT_BOUNCE)
	return etTexture2D(texture_diffuse, projected.xy) * occlusion;
#else
	return occlusion;
#endif
}

void main()
{
	vec4 noiseSample = etTexture2D(texture_noise, NoiseTexCoord);
	vec3 normalSample = decodeNormal(etTexture2D(texture_normal, TexCoord).xy);
	float depthSample = 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0;
	
	vec3 viewSpacePosition = restoreViewSpacePosition(2.0 * TexCoord - 1.0, depthSample);
	
#if (COMPUTE_LIGHT_BOUNCE)
	vec4 environment = vec4(0.0);
#else
	float environment = 0.0;
#endif
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		environment += performRaytracingInViewSpace(viewSpacePosition, normalSample, noiseSample);
		noiseSample = etTexture2D(texture_noise, NoiseTexCoord + noiseSample.yz);
	}
	
#if (COMPUTE_LIGHT_BOUNCE)
	etFragmentOut = environment / float(NUM_SAMPLES);
#else
	etFragmentOut = vec4(environment / float(NUM_SAMPLES));
#endif
}
