#include <et>

/*
 * Inputs / outputs
 */

#include <inputlayout>

struct VSOutput {
	float4 position [[position]];
	float2 texCoord0;
};

struct FSOutput {
	float4 color0 [[color(0)]];
};

struct ObjectVariables {
	float4x4 worldTransform;
};

/*
 * Vertex shader
 */
vertex VSOutput vertexMain(VSInput in [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	VSOutput out;

#if (TRANSFORM_INPUT_POSITION)
	out.position = passVariables.viewProjection * objectVariables.worldTransform * float4(in.position, 1.0);
#else
	out.position = float4(in.position, 1.0);
#endif

	out.texCoord0 = in.texCoord0;
	return out;
}

/*
 * Fragment shader
 */
fragment FSOutput fragmentMain(VSOutput in [[stage_in]],
	texture2d<float> albedoTexture[[texture(0)]],
	sampler albedoSampler[[sampler(0)]])
{
	FSOutput out;
	out.color0 = albedoTexture.sample(albedoSampler, in.texCoord0);
	return out;
}
