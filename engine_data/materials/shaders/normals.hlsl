struct VSOutput 
{
	float4 position : SV_POSITION;
	float3 normal
};

VSOutput vsMain(VSInput vsIn)
{
	VSOutput result;
	result.position = 0.1 * float4(vsIn.position, 1.0);
	result.normal = vsIn.normal;
	return result;
}

float4 psMain(VSOutput fsIn)
{
	return float4(0.5 * fsIn.normal + 0.5, 1.0);
}