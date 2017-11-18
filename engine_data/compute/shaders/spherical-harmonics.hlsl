#include <et>

RWTexture2D<float4> outputImage : DECL_STORAGE(0);

[numthreads(1, 1, 1)]
void computeMain() 
{
	for (uint i = 0; i < 9; ++i)
		outputImage[uint2(0, i)] = float4(float(i) / 8.0, 0.5, 0.25, 1.0);
}
