#include <et>

Texture2D<float4> inputImage : DECL_TEXTURE(BaseColor);
RWTexture2D<float4> outputImage : DECL_STORAGE(1);

struct CSInput
{
	uint3 groupIndex : SV_GroupID;
	uint3 groupThreadIndex : SV_GroupThreadID;
};

cbuffer MaterialVariables : DECL_BUFFER(Material) 
{
	float extraParameters;
};

static const uint ThreadGroupSize = 16;
static const uint numThreads = ThreadGroupSize * ThreadGroupSize;

groupshared float4 sharedMemory[numThreads];

[numThreads(ThreadGroupSize, ThreadGroupSize, 1)]
void computeMain(CSInput input) 
{
	const uint2 sampleIndex = input.groupIndex.xy * 32 + input.groupThreadIndex.xy * 2;
	const uint threadIndex = input.groupThreadIndex.y * ThreadGroupSize + input.groupThreadIndex.x;

	float s0 = inputImage[sampleIndex + uint2(0, 0)].x;
	float s1 = inputImage[sampleIndex + uint2(1, 0)].x;
	float s2 = inputImage[sampleIndex + uint2(0, 1)].x;
	float s3 = inputImage[sampleIndex + uint2(1, 1)].x;
	
	sharedMemory[threadIndex] = float4(s0, s1, s2, s3);
	GroupMemoryBarrierWithGroupSync();

	for (uint s = numThreads / 2; s > 0; s >>= 1)
	{
		if (threadIndex < s)
		{
			sharedMemory[threadIndex] += sharedMemory[threadIndex + s];
		}
		GroupMemoryBarrierWithGroupSync();
	}

	if (threadIndex == 0)
	{
		float value = dot(sharedMemory[0], 0.25f) / numThreads;
		float expValue = exp(value);
		outputImage[input.groupIndex.xy] = lerp(value, expValue, extraParameters);
	}
}
