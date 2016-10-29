#include "common.metal.inl"

/*
 * Inputs / outputs
 */

%built-in-input%

struct VSOutput {
	float4 position [[position]];
	float3 normal;
	float3 toCamera;
	float3 toLight;
};

struct FSOutput {
	float4 color0 [[color(0)]];
};

struct ObjectVariables {
	float4x4 worldTransform;
	float4x4 worldRotationTransform;
};

/*
 * Vertex shader
 */
vertex VSOutput vertexMain(VSInput in [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	float4 transformedPosition = objectVariables.worldTransform * float4(in.position, 1.0);
	VSOutput out;
	out.position = passVariables.viewProjection * transformedPosition;
	out.normal = (objectVariables.worldRotationTransform * float4(in.normal, 0.0)).xyz;
	out.toCamera = (passVariables.cameraPosition - transformedPosition).xyz;
	out.toLight = (passVariables.lightPosition - transformedPosition * passVariables.lightPosition.w).xyz;
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
