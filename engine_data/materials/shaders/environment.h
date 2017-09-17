#if (EQUIRECTANGULAR_ENV_MAP)
Texture2D<float4> environmentTexture : CONSTANT_LOCATION(t, EnvironmentTextureBinding, TexturesSetIndex);
#else
TextureCube<float4> environmentTexture : CONSTANT_LOCATION(t, EnvironmentTextureBinding, TexturesSetIndex);
#endif

SamplerState environmentSampler : CONSTANT_LOCATION(s, EnvironmentSamplerBinding, TexturesSetIndex);

float3 sampleEnvironment(float3 i, in float3 l, float roughness)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	environmentTexture.GetDimensions(0, w, h, levels);
	float sampledLod = roughness * (levels - 1.0);

#if (EQUIRECTANGULAR_ENV_MAP)
	float2 sampleCoord = float2(0.5 * atan2(i.z, i.x), asin(i.y)) / PI + 0.5;
	return environmentTexture.SampleLevel(environmentSampler, sampleCoord, sampledLod).xyz;
#else
	return environmentTexture.SampleLevel(environmentSampler, i, sampledLod).xyz;
#endif
}

float3 specularDominantDirection(in float3 n, in float3 v, in float roughness)
{
	float3 r = -reflect(v, n);
	float smoothness = saturate(1.0 - roughness);
	float factor = smoothness * (sqrt(smoothness) + roughness);
	return normalize(lerp(n, r, factor));
}

float3 diffuseDominantDirection(in float3 n, in float3 v, in float roughness)
{
	float a = 1.02341 * roughness - 1.51174;
	float b = -0.511705 * roughness + 0.755868;
	float factor = saturate((dot(n, v) * a + b) * roughness);
	return normalize(lerp(n, v, factor));
}
