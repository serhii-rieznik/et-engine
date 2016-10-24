#include "common.metal.inl"


/*
 * Inputs / outputs
 */

%built-in-input%

struct VSOutput {
	float4 position [[position]];
	float3 normal;
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
	out.position = passVariables.viewProjection * objectVariables.worldTransform * float4(in.position, 1.0);
	out.normal = in.normal;
	return out;
}

/*
 * Fragment shader
 */
fragment FSOutput fragmentMain(VSOutput in [[stage_in]])
{
	FSOutput out;
	out.color0 = float4(0.5 + 0.5 * normalize(in.normal), 1.0);
	return out;
}
