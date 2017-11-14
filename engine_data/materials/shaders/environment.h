TextureCube<float4> convolvedDiffuseTexture : CONSTANT_LOCATION(t, ConvolvedDiffuseTextureBinding, TexturesSetIndex);
SamplerState convolvedDiffuseSampler : CONSTANT_LOCATION(s, ConvolvedDiffuseSamplerBinding, TexturesSetIndex);

TextureCube<float4> convolvedSpecularTexture : CONSTANT_LOCATION(t, ConvolvedSpecularTextureBinding, TexturesSetIndex);
SamplerState convolvedSpecularSampler : CONSTANT_LOCATION(s, ConvolvedSpecularSamplerBinding, TexturesSetIndex);

float3 sampleEnvironment(float3 i, in float3 l, float roughness)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	convolvedSpecularTexture.GetDimensions(0, w, h, levels);
	float sampledLod = roughness * (levels - 1.0);
	return convolvedSpecularTexture.SampleLevel(convolvedSpecularSampler, i, sampledLod).xyz;
}

float3 sampleDiffuseConvolution(float3 n)
{
	return convolvedDiffuseTexture.SampleLevel(convolvedDiffuseSampler, n, 0.0).xyz;
}

float3 sampleSpecularConvolution(float3 i, float roughness)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	convolvedSpecularTexture.GetDimensions(0, w, h, levels);
	float sampledLod = roughness * (levels - 1.0);
	return convolvedSpecularTexture.SampleLevel(convolvedSpecularSampler, i, sampledLod).xyz;
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
