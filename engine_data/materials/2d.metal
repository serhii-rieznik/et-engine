struct VSOutput
{
	float4 position [[position]];
	float2 texCoord;
};

vertex VSOutput vertexMain(constant float2* position [[buffer(0)]], uint vertexId [[vertex_id]])
{
	VSOutput vo;
	vo.position = float4(position[vertexId], 0.0, 1.0);
	vo.texCoord = 0.5 * position[vertexId] + 0.5;
	return vo;
}

fragment float4 fragmentMain(VSOutput vertexOut [[stage_in]], texture2d<float> color_texture [[texture(0)]])
{
	constexpr sampler defaultSampler;
	return color_texture.sample(defaultSampler, vertexOut.texCoord);
}
