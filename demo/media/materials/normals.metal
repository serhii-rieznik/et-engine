struct VSInput
{
	packed_float3 position [[attribute(0)]];
	packed_float3 normal [[attribute(1)]];
};

struct VSOutput
{
	float4 position [[position]];
	float3 normal;
};

struct Variables
{
	float4x4 viewProjection;
	float4x4 transform;
	float3 onotole;
	float4x4 shit;
};

vertex VSOutput vertex_main(constant VSInput* vsInput [[buffer(0)]],
							constant Variables& variables [[buffer(1)]],
							uint vertexId [[vertex_id]])
{
	VSOutput result;
	result.position = variables.viewProjection * variables.transform * float4(vsInput[vertexId].position, 1.0);
	result.normal = (variables.transform * float4(vsInput[vertexId].normal, 0.0)).xyz;
	return result;
}

fragment float4 fragment_main(VSOutput fsInput [[stage_in]])
{
	return float4(0.5f + 0.5f * normalize(fsInput.normal), 1.0);
}
