#include <et>

TextureCube<float4> baseColorTexture : DECL_TEXTURE(BaseColor);
SamplerState baseColorSampler : DECL_SAMPLER(BaseColor);

RWTexture2D<float4> outputImage : DECL_STORAGE(0);

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
		   atan2(x1 * y1, sqrt(x11sq + y11sq + 1.0)) ;
}

[numthreads(1, 1, 1)]
void computeMain() 
{
	/*
	outputImage[uint2(0, 0)] = float4(0.38f, 0.43f, 0.45f, 0.0f);
	outputImage[uint2(0, 1)] = float4(0.29f, 0.36f, 0.41f, 0.0f);
	outputImage[uint2(0, 2)] = float4(0.04f, 0.03f, 0.01f, 0.0f);
	outputImage[uint2(0, 3)] = float4(-0.10f, -0.10f, -0.09f, 0.0f);
	outputImage[uint2(0, 4)] = float4(-0.06f, -0.06f, -0.04f, 0.0f);
	outputImage[uint2(0, 5)] = float4(.01f, -0.01f, -0.05f, 0.0f);
	outputImage[uint2(0, 6)] = float4(-0.09f, -0.13f, -0.15f, 0.0f);
	outputImage[uint2(0, 7)] = float4(-0.06f, -0.05f, -0.04f, 0.0f);
	outputImage[uint2(0, 8)] = float4(0.02f, -0.00f, -0.05f, 0.0f);
	*/

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
	
	const float3 t2[6] = {
		float3(0.0, 0.0, 1.0),
		float3(1.0, 0.0, 0.0),
		float3(0.0, 1.0, 0.0),
	};
	
	const uint sampledLevel = 3;

	uint level0Width = 0;
	uint level0Height = 0;
	baseColorTexture.GetDimensions(level0Width, level0Height);
	
	uint w = level0Width >> sampledLevel;
	uint h = level0Height >> sampledLevel;
	float invFaceSize = 1.0 / float(min(w, h));

	float3 sh[9] = { 
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0),
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0),
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0), 
		float3(0.0, 0.0, 0.0)
	};

	float passedSamples = 0.0;
	float3 integralResult = 0.0;
	for (uint face = 0; face < 6; ++face)
	{
		float3 a0 = t0[face];
		float3 a1 = t1[face / 2];
		float3 a2 = t2[face / 2];
		
		for (uint y = 0; y < h; ++y)
		{
			float v = (float(y) / float(h - 1)) * 2.0 - 1.0;
			for (uint x = 0; x < w; ++x)
			{
				float u = (float(x) / float(w - 1)) * 2.0 - 1.0;
				float solidAngle = texelSolidAngle(u, v, invFaceSize);
				float3 direction = normalize(a0 + a1 * u + a2 * v);
				float3 radiance = baseColorTexture.SampleLevel(baseColorSampler, direction, sampledLevel).xyz;

			    sh[0] += (1.0) * 0.282095 * radiance * solidAngle;
    			sh[1] += (2.0/3.0) * 0.488603 * direction.y * radiance * solidAngle;
    			sh[2] += (2.0/3.0) * 0.488603 * direction.z * radiance * solidAngle;
    			sh[3] += (2.0/3.0) * 0.488603 * direction.x * radiance * solidAngle;
    			sh[4] += (1.0 / 4.0) * 1.092548 * direction.x * direction.y * radiance * solidAngle;
    			sh[5] += (1.0 / 4.0) * 1.092548 * direction.y * direction.z * radiance * solidAngle;
    			sh[6] += (1.0 / 4.0) * 0.315392 * (3.0 * direction.z * direction.z - 1.0) * radiance * solidAngle;
    			sh[7] += (1.0 / 4.0) * 1.092548 * direction.x * direction.z * radiance * solidAngle;
    			sh[8] += (1.0 / 4.0) * 0.546274 * (direction.x * direction.x - direction.y * direction.y) * radiance * solidAngle;
			}
		}	
	}
	
	for (uint i = 0; i < 9; ++i)
		outputImage[uint2(0, i)] = float4(sh[i], 0.0);
}
