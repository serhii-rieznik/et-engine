static const float2 PoissonDistribution[8] = {
	float2( 0.8528466f,  0.0213828f),
	float2( 0.1141956f,  0.2880972f),
	float2( 0.5853493f, -0.6930891f),
	float2( 0.6362274f,  0.7029839f),
	float2(-0.1640182f, -0.4143998f),
	float2(-0.8862001f, -0.3506839f),
	float2(-0.2186042f,  0.8690619f),
	float2(-0.8200445f,  0.4156708f)
};
static const float ShadowMapBias = 0.005;
static const float PCFShadowRadius = PI / 2.0;

float sampleShadow(in float3 shadowTexCoord, in float rotationKernel, in float2 shadowmapSize)
{
	float biasedZ = shadowTexCoord.z - ShadowMapBias;
	float2 scaledUV = shadowTexCoord.xy * 0.5 + 0.5;
	float shadow = shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV, biasedZ);

#if (EnablePCFShadow)
	float angle = 2.0 * PI * (rotationKernel + 2.0 * PI * continuousTime);
	float sn = sin(angle);
	float cs = cos(angle);
	for (uint i = 0; i < 8; ++i)
	{
		float2 o = PoissonDistribution[i] / shadowmapSize;
		float2 r = PCFShadowRadius * float2(dot(o, float2(cs, -sn)), dot(o, float2(sn,  cs)));
		shadow += shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV + r, biasedZ);
	}
	shadow /= 9.0;
#endif

	return shadow;

}
