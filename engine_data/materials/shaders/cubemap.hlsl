#include <et>
#include <inputdefines>
#include <inputlayout>
#include "importance-sampling.h"
#include "bsdf.h"

#if (EQ_MAP_TO_CUBEMAP)
	Texture2D<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
#elif (VISUALIZE_CUBEMAP || SPECULAR_CONVOLUTION)
	TextureCube<float4> baseColorTexture : CONSTANT_LOCATION(t, BaseColorTextureBinding, TexturesSetIndex);
#endif
SamplerState baseColorSampler : CONSTANT_LOCATION(s, BaseColorSamplerBinding, TexturesSetIndex);

cbuffer ObjectVariables : CONSTANT_LOCATION(b, ObjectVariablesBufferIndex, VariablesSetIndex)
{
	row_major float4x4 worldTransform;
};

cbuffer MaterialVariables : CONSTANT_LOCATION(b, MaterialVariablesBufferIndex, VariablesSetIndex)
{
	float4 roughnessScale;
};

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
	float3 direction : TEXCOORD1;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.position = float4(vsIn.position, 1.0);
	vsOut.direction = mul(vsOut.position, worldTransform).xyz;

#if (VISUALIZE_CUBEMAP)
	vsOut.position = mul(vsOut.position, worldTransform);
#endif

	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
#if (VISUALIZE_CUBEMAP)	

	float phi = fsIn.texCoord0.x * 2.0 * PI - PI;
	float theta = fsIn.texCoord0.y * PI - 0.5 * PI;
	float sinTheta = sin(theta);
	float cosTheta = cos(theta);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);
	float3 sampleDirection = float3(cosPhi * cosTheta, sinTheta, sinPhi * cosTheta);
	return baseColorTexture.SampleLevel(baseColorSampler, sampleDirection, roughnessScale.x);

#elif (EQ_MAP_TO_CUBEMAP)

	float3 d = normalize(fsIn.direction);        	
	float u = atan2(d.z, d.x) * 0.5 / PI + 0.5;
	float v = asin(d.y) / PI + 0.5;
	return baseColorTexture.SampleLevel(baseColorSampler, float2(u, v), 0.0);

#elif (SPECULAR_CONVOLUTION)

	float3 n = normalize(fsIn.direction);
	float3 v = n;
	
	float3 up = abs(n.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
	float3 tX = normalize(cross(up, n));
	float3 tY = cross(n, tX);

#define DIFFUSE_CONVOLUTION 0

#if (DIFFUSE_CONVOLUTION)
	#define samples 512
#else
	#define samples 512
	float roughness = roughnessScale.x / 8.0;
	roughness = max(0.0001, roughness * roughness);
#endif

	float3 result = 0.0;
	float weight = 0.0;
	for (uint i = 0; i < samples; ++i)
	{
		float2 Xi = hammersley(i, samples);
		                                   
	#if (DIFFUSE_CONVOLUTION)
		float3 h = importanceSampleCosine(Xi);	
	#else
		float3 h = importanceSampleGGX(Xi, roughness);	
	#endif

		h = tX * h.x + tY * h.y + n * h.z;
		float3 l = 2.0 * dot(v, h) * h - v;
		float LdotN = saturate(dot(l, n));

	#if (DIFFUSE_CONVOLUTION)
		float pdf = LdotN / PI;
		float w = 1.0;
	#else
		float NdotH = dot(n, h);
		float LdotH = saturate(dot(l, h));
		float pdf = ggxDistribution(NdotH, roughness) * NdotH / (4.0 * LdotH);
		float w = LdotN;
	#endif
		
		float omegaS = 1.0 / ((float)(samples) * pdf);
		float omegaP = 4.0 * PI / (6.0 * roughnessScale.y * roughnessScale.z);
		float level = 0.5 * log2(omegaS / omegaP);

		result += w * baseColorTexture.SampleLevel(baseColorSampler, l, level).xyz;
		weight += w;
	}
	result /= max(0.0001, weight);
	return float4(result, 1.0);

#else

	return 1.0;

#endif
}
