#include "shaders.inl"

struct VSOutput
{
	FLOAT(4) position [[position]];
	FLOAT(2) texCoord0;
};

vertex VSOutput vertexMain(VSInput vsInput [[stage_in]])
{
	VSOutput vo;
	vo.position = float4(vsInput.position, 0.0, 1.0);
	vo.texCoord0 = 0.5 * vsInput.position + 0.5;
	return vo;
}

fragment float4 fragmentMain(VSOutput vertexOut [[stage_in]],
	texture2d<float> albedoTexture [[texture(0)]],
	sampler albedoSampler [[sampler(0)]])
{
	return albedoTexture.sample(albedoSampler, vertexOut.texCoord0);
}
