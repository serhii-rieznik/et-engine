#include <et>

layout (std140, set = 0, location = PassVariablesBufferIndex) uniform PassVariabless passVariabless;

layout (std140, set = 0, location = ObjectVariablesBufferIndex) uniform ObjectVariables {
	mat4 worldTransform;
} objectVariables;

struct VSOutput {
	vec2 texCoord0;
};

#include <inputdefines>
#include <stagedefine>

#if defined(ET_VERTEX_SHADER)

#include <inputlayout>
layout(location = 0) out VSOutput vsOut;

void main()
{
	vsOut.texCoord0 = texCoord0;

#if (TRANSFORM_INPUT_POSITION)
	gl_Position = passVariables.viewProjection * objectVariables.worldTransform * vec4(position, 1.0);
#else
	gl_Position = vec4(position, 1.0);
#endif
}

#elif defined(ET_FRAGMENT_SHADER)

//	texture2d<float> albedoTexture[[texture(0)]],
//	sampler albedoSampler[[sampler(0)]])

layout(location = 0) in VSOutput vsOut;
layout(location = 0) out vec4 outColor0;

void main()
{
	outColor0 = vec4(1.0); // albedoTexture.sample(albedoSampler, in.texCoord0);
}

#else
#
#error Invalid or unsupported shader
#
#endif
