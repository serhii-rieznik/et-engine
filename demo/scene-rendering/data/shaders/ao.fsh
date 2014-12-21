uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;
uniform sampler2D texture_noise;

etFragmentIn vec2 NormalizedTexCoord;
etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NoiseTexCoord;

#define	NUM_SAMPLES			48
#define SAMPLE_SIZE			75.0

#include "viewspace.fsh"

vec3 randomVectorOnHemisphere(in vec3 normal, in vec3 noise)
{
	vec3 n = normalize(noise - 0.5);
	return n * sign(dot(n, normal));
}

vec4 performRaytracingInViewSpace(in vec3 vp, in vec3 vn, in vec4 noise)
{
	vec3 randomNormal = randomVectorOnHemisphere(vn, noise.xyz);
	vec3 projected = projectViewSpacePosition(vp - randomNormal * (noise.w * SAMPLE_SIZE));
	float sampledDepth = etTexture2D(texture_depth, projected.xy).x;
	
	if (sampledDepth > projected.z)
		return vec4(0.0);
	
	float invSampledDepth = inversesqrt(1.0 - sampledDepth);
	float invProjectedDepth = inversesqrt(1.0 - projected.z);
	float depthDifference = invProjectedDepth - invSampledDepth;
	float occlusion = (1.0 - noise.w) / (1.0 + depthDifference * depthDifference);
	
	return etTexture2D(texture_diffuse, projected.xy) * occlusion;
}

void main()
{
	float depthSample = 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0;
	
	vec3 viewSpacePosition = restoreViewSpacePosition(NormalizedTexCoord, depthSample);
	vec3 normalSample = normalize(normalBias * etTexture2D(texture_normal, TexCoord).xyz - normalScale);
	vec4 noiseSample = etTexture2D(texture_noise, NoiseTexCoord);
	
	vec4 environment = vec4(0.0);
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		environment += performRaytracingInViewSpace(viewSpacePosition, normalSample, noiseSample);
		noiseSample = etTexture2D(texture_noise, NoiseTexCoord + noiseSample.yz);
	}
	
	etFragmentOut = environment / float(NUM_SAMPLES);
}
