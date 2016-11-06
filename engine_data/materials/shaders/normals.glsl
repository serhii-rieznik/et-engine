#include <et>

layout (std140, set = 0, location = PassVariablesBufferIndex) uniform PassVariabless passVariabless;

layout (std140, set = 0, location = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
	mat4 worldRotationTransform;	
} objectVariables;

struct VSOutput {
	vec3 normal;
};

#include <inputdefines>
#include <stagedefine>

#if defined(ET_VERTEX_SHADER)

layout (location = 0) in vec3 position; 
layout (location = 1) in vec3 normal; 
layout (location = 0) out VSOutput vsOut;

void main()
{
	vsOut.normal = (objectVariables.worldRotationTransform * vec4(normal, 0.0)).xyz;
	gl_Position = passVariabless.viewProjection * objectVariables.worldTransform * vec4(position, 1.0);
}

#elif defined(ET_FRAGMENT_SHADER)

layout (location = 0) in VSOutput vsOut;
layout (location = 0) out vec4 outColor0;

void main()
{
	outColor0 = vec4(0.5 + 0.5 * normalize(vsOut.normal), 1.0);
}

#else
#
#	error Invalid or unsupported shader
#
#endif