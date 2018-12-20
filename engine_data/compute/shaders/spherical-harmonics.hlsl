#include <et>

TextureCube<float4> inputTexture : DECLARE_TEXTURE;
RWTexture2D<float4> outputImage : DECLARE_STORAGE;

struct CSInput
{
	uint3 groupThreadIndex : SV_GroupThreadID;
};

float texelSolidAngle(float u, float v, float invSize) 
{
	float x0 = u - invSize;
	float x1 = u + invSize;
	float y0 = v - invSize;
	float y1 = v + invSize;
	float x00sq = x0 * x0;
	float x11sq = x1 * x1;
	float y00sq = y0 * y0;
	float y11sq = y1 * y1;
	return atan2(x0 * y0, sqrt(x00sq + y00sq + 1.0)) -
		atan2(x0 * y1, sqrt(x00sq + y11sq + 1.0)) -
		atan2(x1 * y0, sqrt(x11sq + y00sq + 1.0)) +
		atan2(x1 * y1, sqrt(x11sq + y11sq + 1.0));
}

const float3 t0[6] = {
	float3(1.0, 0.0, 0.0),
	float3(-1.0, 0.0, 0.0),
	float3(0.0, 1.0, 0.0),
	float3(0.0, -1.0, 0.0),
	float3(0.0, 0.0, 1.0),
	float3(0.0, 0.0, -1.0),
};

const float3 t1[6] = {
	float3(0.0, 1.0, 0.0),
	float3(0.0, 0.0, 1.0),
	float3(1.0, 0.0, 0.0),
};

#define shOrder 9

static const float shProjection0 = ((1.0 / 1.0) * 0.282095);
static const float shProjection1 = ((2.0 / 3.0) * 0.488603);
static const float shProjection2 = ((2.0 / 3.0) * 0.488603);
static const float shProjection3 = ((2.0 / 3.0) * 0.488603);
static const float shProjection4 = ((1.0 / 4.0) * 1.092548);
static const float shProjection5 = ((1.0 / 4.0) * 1.092548);
static const float shProjection6 = ((1.0 / 4.0) * 0.315392);
static const float shProjection7 = ((1.0 / 4.0) * 1.092548);
static const float shProjection8 = ((1.0 / 4.0) * 0.546274);

groupshared float3 sphericalHarmonics0 = 0.0;
groupshared float3 sphericalHarmonics1 = 0.0;
groupshared float3 sphericalHarmonics2 = 0.0;
groupshared float3 sphericalHarmonics3 = 0.0;
groupshared float3 sphericalHarmonics4 = 0.0;
groupshared float3 sphericalHarmonics5 = 0.0;
groupshared float3 sphericalHarmonics6 = 0.0;
groupshared float3 sphericalHarmonics7 = 0.0;
groupshared float3 sphericalHarmonics8 = 0.0;

[numthreads(6, 1, 1)]
void computeMain(CSInput input) 
{
	const uint sampledLevel = 0;

	uint level0Width = 0;
	uint level0Height = 0;
	inputTexture.GetDimensions(level0Width, level0Height);

	uint w = max(1, level0Width >> sampledLevel);
	uint h = max(1, level0Height >> sampledLevel);
	float invFaceSize = 1.0 / float(min(w, h));

	float3 sh0 = 0.0;
	float3 sh1 = 0.0;
	float3 sh2 = 0.0;
	float3 sh3 = 0.0;
	float3 sh4 = 0.0;
	float3 sh5 = 0.0;
	float3 sh6 = 0.0;
	float3 sh7 = 0.0;
	float3 sh8 = 0.0;

	uint face = uint(input.groupThreadIndex.x);

	float3 a0 = t0[face];
	float3 a1 = t1[face / 2];
	float3 a2 = cross(a0, a1);

	for (uint y = 0; y < h; ++y)
	{
		float v = (float(y) / float(h - 1)) * 2.0 - 1.0;
		for (uint x = 0; x < w; ++x)
		{
			float u = (float(x) / float(w - 1)) * 2.0 - 1.0;
			float solidAngle = texelSolidAngle(u, v, invFaceSize);
			float3 direction = normalize(a0 + a1 * u + a2 * v);
			float3 radiance = inputTexture.SampleLevel(LinearWrap, direction, sampledLevel).xyz;
			sh0 += radiance * (solidAngle);
			sh1 += radiance * (solidAngle  * direction.y);
			sh2 += radiance * (solidAngle  * direction.z);
			sh3 += radiance * (solidAngle  * direction.x);
			sh4 += radiance * (solidAngle  * direction.x * direction.y);
			sh5 += radiance * (solidAngle  * direction.y * direction.z);
			sh6 += radiance * (solidAngle  * (3.0 * direction.z * direction.z - 1.0));
			sh7 += radiance * (solidAngle  * direction.x * direction.z);
			sh8 += radiance * (solidAngle  * (direction.x * direction.x - direction.y * direction.y));
		}
	}

	sh0 *= shProjection0;
	sh1 *= shProjection1;
	sh2 *= shProjection2;
	sh3 *= shProjection3;
	sh4 *= shProjection4;
	sh5 *= shProjection5;
	sh6 *= shProjection6;
	sh7 *= shProjection7;
	sh8 *= shProjection8;

	GroupMemoryBarrierWithGroupSync();

	for (uint f = 0; f < 6; ++f)
	{
		if (face == f)
		{
			if (face == 5)
			{
				outputImage[uint2(0, 0)] = float4(sphericalHarmonics0 + sh0, 1.0);
				outputImage[uint2(1, 0)] = float4(sphericalHarmonics1 + sh1, 1.0);
				outputImage[uint2(2, 0)] = float4(sphericalHarmonics2 + sh2, 1.0);
				outputImage[uint2(3, 0)] = float4(sphericalHarmonics3 + sh3, 1.0);
				outputImage[uint2(4, 0)] = float4(sphericalHarmonics4 + sh4, 1.0);
				outputImage[uint2(5, 0)] = float4(sphericalHarmonics5 + sh5, 1.0);
				outputImage[uint2(6, 0)] = float4(sphericalHarmonics6 + sh6, 1.0);
				outputImage[uint2(7, 0)] = float4(sphericalHarmonics7 + sh7, 1.0);
				outputImage[uint2(8, 0)] = float4(sphericalHarmonics8 + sh8, 1.0);
			}
			else
			{
				sphericalHarmonics0 += sh0;
				sphericalHarmonics1 += sh1;
				sphericalHarmonics2 += sh2;
				sphericalHarmonics3 += sh3;
				sphericalHarmonics4 += sh4;
				sphericalHarmonics5 += sh5;
				sphericalHarmonics6 += sh6;
				sphericalHarmonics7 += sh7;
				sphericalHarmonics8 += sh8;
			}
		}

		GroupMemoryBarrierWithGroupSync();
	}
}
