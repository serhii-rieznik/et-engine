uniform sampler2D texture_color;
uniform sampler2D texture_normal;
uniform sampler2D texture_depth;

uniform vec2 clipPlanes;

#define NUM_SAMPLES	4
#define DEPTH_DIFFERENCE_SCALE	0.66666

etFragmentIn vec2 CenterTexCoord;
etFragmentIn vec3 NextTexCoords[NUM_SAMPLES];
etFragmentIn vec3 PreviousTexCoords[NUM_SAMPLES];

float restoreViewSpaceDistance(in float depth)
{
	return (clipPlanes.x * clipPlanes.y) / (clipPlanes.y - depth * (clipPlanes.y - clipPlanes.x));
}

void main()
{
	vec3 centerNormalSample = 2.0 * etTexture2D(texture_normal, CenterTexCoord).xyz - 1.0;
	float centerDepthSample = restoreViewSpaceDistance(2.0 * etTexture2D(texture_depth, CenterTexCoord).x - 1.0);
	
	float totalWeight = 1.0;
	vec4 result = etTexture2D(texture_color, CenterTexCoord);
	
//*
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		float nextDepth = restoreViewSpaceDistance(2.0 * etTexture2D(texture_depth, NextTexCoords[i].xy).x - 1.0);
		vec3 nextNormal = 2.0 * etTexture2D(texture_normal, NextTexCoords[i].xy).xyz - 1.0;
		vec4 nextColor = etTexture2D(texture_color, NextTexCoords[i].xy);
		
		float prevDepth = restoreViewSpaceDistance(2.0 * etTexture2D(texture_depth, PreviousTexCoords[i].xy).x - 1.0);
		vec3 prevNormal = 2.0 * etTexture2D(texture_normal, PreviousTexCoords[i].xy).xyz - 1.0;
		vec4 prevColor = etTexture2D(texture_color, PreviousTexCoords[i].xy);
		
		float nextDepthWeight = exp(-DEPTH_DIFFERENCE_SCALE * abs(nextDepth - centerDepthSample));
		float prevDepthWeight = exp(-DEPTH_DIFFERENCE_SCALE * abs(prevDepth - centerDepthSample));
		
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
