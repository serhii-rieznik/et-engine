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

static const float shProjection[9] = {
	((1.0 / 1.0) * 0.282095),
	((2.0 / 3.0) * 0.488603),
	((2.0 / 3.0) * 0.488603),
	((2.0 / 3.0) * 0.488603),
	((1.0 / 4.0) * 1.092548),
	((1.0 / 4.0) * 1.092548),
	((1.0 / 4.0) * 0.315392),
	((1.0 / 4.0) * 1.092548),
	((1.0 / 4.0) * 0.546274),
};

groupshared float3 sphericalHarmonics[9];

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

	float3 sh[9] = { };

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
			sh[0] += radiance * (solidAngle);
			sh[1] += radiance * (solidAngle  * direction.y);
			sh[2] += radiance * (solidAngle  * direction.z);
			sh[3] += radiance * (solidAngle  * direction.x);
			sh[4] += radiance * (solidAngle  * direction.x * direction.y);
			sh[5] += radiance * (solidAngle  * direction.y * direction.z);
			sh[6] += radiance * (solidAngle  * (3.0 * direction.z * direction.z - 1.0));
			sh[7] += radiance * (solidAngle  * direction.x * direction.z);
			sh[8] += radiance * (solidAngle  * (direction.x * direction.x - direction.y * direction.y));
		}
	}

	for (uint i = 0; i < 9; ++i)
		sh[i] *= shProjection[i];

	GroupMemoryBarrierWithGroupSync();

	for (uint f = 0; f < 6; ++f)
	{
		if (face == f)
		{
			if (face == 5)
			{
				for (uint j = 0; j < 9; ++j)
					outputImage[uint2(0, j)] = float4(sphericalHarmonics[j] + sh[j], 0.0);
			}
			else
			{
				for (uint j = 0; j < 9; ++j)
					sphericalHarmonics[j] += sh[j];
			}
		}

		GroupMemoryBarrierWithGroupSync();
	}

}
