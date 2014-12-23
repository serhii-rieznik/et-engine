uniform sampler2D texture_depth;
uniform sampler2D texture_diffuse;

uniform float motionDistanceScale;
uniform mat4 mModelViewInverseToPrevious;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 NormalizedTexCoord;

#define NUM_SAMPLES	16

void main()
{
	vec4 previousProjection = mModelViewInverseToPrevious *
		vec4(NormalizedTexCoord, 2.0 * etTexture2D(texture_depth, TexCoord).x - 1.0, 1.0);
	
	vec2 motion = NormalizedTexCoord - previousProjection.xy / previousProjection.w;
	float motionLength = length(motion);
	motion /= motionLength;
	
	float dMotion = (motionDistanceScale * motionLength) / float(NUM_SAMPLES);
	
	vec4 colorSample = vec4(0.0);
	vec2 sampleTexCoord = TexCoord - 0.5 * motionLength * motion;
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		colorSample += etTexture2D(texture_diffuse, sampleTexCoord);
		sampleTexCoord += motion * dMotion;
	}
	
	etFragmentOut = colorSample / float(NUM_SAMPLES);
}
