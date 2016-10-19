#include "common.metal.inl"
#include "input.metal.inl"
#include "output.metal.inl"

vertex VSOutput vertexMain(VSInput in [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	VSOutput out;
	out.position = objectVariables.worldTransform * float4(vsInput.position, 1.0);

#if (TEXCOORD0_SIZE > 0)
	out.texCoord0 = in.texCoord0;
#endif

	return out;
}

fragment FSOutput fragmentMain(VSOutput in [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]])
{
	FSOutput out;
	out.color0 = float4(in.texCoord0, 0.25f, 1.0f);
	return out;
}
