#include <et>

layout (std140, set = 0, binding = PassVariablesBufferIndex) uniform PassVariables passVariables;

layout (std140, set = 0, binding = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
	mat4 worldRotationTransform;	
} objectVariables;

layout(binding = 0, set = 1) uniform sampler2D albedoTexture;

struct VSOutput {
	vec3 normal;
	vec2 texCoord0;
};

#include <inputdefines>
#include <stagedefine>

#if defined(ET_VERTEX_SHADER)

#include <inputlayout>

layout (location = 0) out VSOutput vsOut;


void main()
{
	vsOut.normal = (objectVariables.worldRotationTransform * vec4(normal, 0.0)).xyz;
	vsOut.texCoord0 = texCoord0;
	gl_Position = passVariables.viewProjection * objectVariables.worldTransform * vec4(position, 1.0);
}

#elif defined(ET_FRAGMENT_SHADER)

layout (location = 0) in VSOutput fsIn;
layout (location = 0) out vec4 outColor0;

void main()
{
	outColor0 = vec4(0.25 + 0.25 * fsIn.normal, 0.0) + 0.5 * texture(albedoTexture, fsIn.texCoord0);
}

#else
#
#	error Invalid or unsupported shader
#
#endif