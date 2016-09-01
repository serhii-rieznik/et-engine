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

struct Uniforms
{
	float4x4 viewProjection;
	float4x4 transform;
};

vertex VSOutput vertex_main(device VSInput* vsInput [[buffer(0)]],
							constant Uniforms& uniforms [[buffer(1)]],
							uint vertexId [[vertex_id]])
{
	VSOutput result;
	result.position = uniforms.viewProjection * uniforms.transform * float4(vsInput[vertexId].position, 1.0);
	result.normal = (uniforms.transform * float4(vsInput[vertexId].normal, 0.0)).xyz;
	return result;
}

fragment float4 fragment_main(VSOutput fsInput [[stage_in]])
{
	return float4(0.5f + 0.5f * normalize(fsInput.normal), 1.0);
}
