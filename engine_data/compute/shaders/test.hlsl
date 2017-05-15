#include <et>

RWTexture2D<float4> outputTest : DECL_STORAGE(StorageBuffer0);

Texture2D<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

struct CSInput
{
	uint3 groupId : SV_GroupID;
    uint3 groupThreadId : SV_GroupThreadID;
    uint3 dispatchThreadId : SV_DispatchThreadID;
    uint groupIndex : SV_GroupIndex;
};

void computeMain(CSInput input)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	baseColorTexture.GetDimensions(0, w, h, levels);
	float u = float(input.dispatchThreadId.x) / w;
	float v = float(input.dispatchThreadId.y) / h;

	float3 color = baseColorTexture.SampleLevel(baseColorSampler, float2(u, v), 0.0).xyz;
	float lum = saturate(dot(color, float3(0.3, 0.58, 0.11)));

	uint2 dst = uint2(uint(255 * lum), 0);
	outputTest[dst] += 1.9214771412037037037037037037037e-4;
}
