#include <et>

layout (std140, set = VariablesSetIndex, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

layout (std140, set = VariablesSetIndex, binding = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
} objectVariables;

#include <inputdefines>
#include <stagedefine>

#if defined(ET_VERTEX_SHADER)

#include <inputlayout>

void main()
{
	vec4 transformedPosition = objectVariables.worldTransform * vec4(position, 1.0);
	gl_Position = passVariables.viewProjection * transformedPosition;
}

#elif defined(ET_FRAGMENT_SHADER)

void main() { }

#else
#
#	error Invalid or unsupported shader
#
#endif