struct VSInput
{
	packed_float3 position;
	packed_float3 normal;
};

struct VSOutput
{
	float4 position [[position]];
	float3 normal;
};

vertex VSOutput vertex_main(device VSInput* vsInput [[buffer(0)]], uint vertexId [[vertex_id]])
{
	VSOutput result;
	result.position = float4(0.1f * vsInput[vertexId].position, 1.0);
	result.normal = vsInput[vertexId].normal;
	return result;
}

fragment float4 fragment_main(VSOutput fsInput [[stage_in]])
{
	return float4(0.5f + 0.5f * normalize(fsInput.normal), 1.0);
}
