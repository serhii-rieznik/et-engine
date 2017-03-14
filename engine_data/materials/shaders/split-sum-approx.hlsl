#include <et>
#include <inputdefines>
#include <inputlayout>
#include "importance-sampling.h"
#include "bsdf.h"

struct VSOutput 
{
	float4 position : SV_Position;
	float2 texCoord0 : TEXCOORD0;
};

VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.texCoord0;
	vsOut.position = float4(vsIn.position, 1.0);
	return vsOut;
}

float4 fragmentMain(VSOutput fsIn) : SV_Target0
{
#define samples 512

	float roughness = fsIn.texCoord0.x;
	float NdotV = fsIn.texCoord0.y;

	float3 n = float3(0.0, 0.0, 1.0);
	float3 v = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

	float3 tX = float3(1.0, 0.0, 0.0);
	float3 tY = float3(0.0, 1.0, 0.0);

	float3 brdf = 0.0;

	for (uint i = 0; i < samples; ++i)
	{
		float2 Xi = hammersley(i, samples);
		float3 h = importanceSampleGGX(Xi, roughness);
		h = tX * h.x + tY * h.y + n * h.z;
		float3 l = 2.0 * dot(v, h) * h - v;
		float LdotN = saturate(dot(l, n));
		float NdotH = saturate(dot(n, h));
		float LdotH = saturate(dot(l, h));
		
		float d = ggxDistribution(NdotH, roughness * roughness);
		float g = ggxMasking(NdotV, LdotN, roughness * roughness);
		float f = pow(1.0 - LdotH, 5.0);
		float pdf = d * NdotH / (4.0 * LdotH);
		float brdf_over_pdf = g * LdotH / (NdotV * NdotH);
		brdf.x += (1.0 - f) * brdf_over_pdf;
		brdf.y += f * brdf_over_pdf;

		h = importanceSampleCosine(Xi);
		h = tX * h.x + tY * h.y + n * h.z;
		l = 2.0 * dot(v, h) * h - v;
		LdotN = saturate(dot(l, n));
		LdotH = saturate(dot(l, h));
		brdf.z += diffuseBurley(LdotN, NdotV, LdotH, roughness * roughness);
	}
	
	return float4(brdf / samples, 1.0); 
}
