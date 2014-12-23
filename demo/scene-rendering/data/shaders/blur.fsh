uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;

#define NUM_SAMPLES				6
#define DEPTH_DIFFERENCE_SCALE	0.05

#include "include/normals.fsh"
#include "include/viewspace.fsh"

etFragmentIn vec2 CenterTexCoord;
etFragmentIn vec3 NextTexCoords[NUM_SAMPLES];
etFragmentIn vec3 PreviousTexCoords[NUM_SAMPLES];

void main()
{
	vec3 centerNormalSample = decodeNormal(etTexture2D(texture_normal, CenterTexCoord).xy);
	float centerDepthSample = DEPTH_DIFFERENCE_SCALE / (1.0 - etTexture2D(texture_depth, CenterTexCoord).x);
	
	float totalWeight = 1.0;
	vec4 result = etTexture2D(texture_color, CenterTexCoord);
	
//*
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		float nextDepth = DEPTH_DIFFERENCE_SCALE / (1.0 - etTexture2D(texture_depth, NextTexCoords[i].xy).x);
		float prevDepth = DEPTH_DIFFERENCE_SCALE / (1.0 - etTexture2D(texture_depth, PreviousTexCoords[i].xy).x);
		
		vec3 nextNormal = decodeNormal(etTexture2D(texture_normal, NextTexCoords[i].xy).xy);
		vec3 prevNormal = decodeNormal(etTexture2D(texture_normal, PreviousTexCoords[i].xy).xy);
		
		vec4 nextColor = etTexture2D(texture_color, NextTexCoords[i].xy);
		vec4 prevColor = etTexture2D(texture_color, PreviousTexCoords[i].xy);
		
		float nextDepthWeight = exp(-abs(nextDepth - centerDepthSample));
		float prevDepthWeight = exp(-abs(prevDepth - centerDepthSample));
		
		float nextNormalWeight = max(0.0, dot(nextNormal, centerNormalSample));
		float prevNormalWeight = max(0.0, dot(prevNormal, centerNormalSample));
		
		float nextWeight = nextDepthWeight * nextNormalWeight;
		float prevWeight = prevDepthWeight * prevNormalWeight;
		
		result += nextColor * nextWeight + prevColor * prevWeight;
		totalWeight += nextWeight + prevWeight;
	}
//*/
	
	etFragmentOut = result / totalWeight;
}
