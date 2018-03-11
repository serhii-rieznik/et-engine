#include <et>

Texture2D<float4> baseColor : DECLARE_TEXTURE;
RWTexture2D<float4> outputTest : DECLARE_STORAGE;

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
	baseColor.GetDimensions(0, w, h, levels);
	float u = float(input.dispatchThreadId.x) / w;
	float v = float(input.dispatchThreadId.y) / h;

	float3 color = baseColor.SampleLevel(LinearClamp, float2(u, v), 0.0).xyz;
	float lum = saturate(dot(color, float3(0.3, 0.58, 0.11)));

	uint2 dst = uint2(uint(255 * lum), 0);
	outputTest[dst] += 1.9214771412037037037037037037037e-4;
}
