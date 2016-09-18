#version 450

layout (set = 0, binding = 0) uniform Variables {
	mat4 matWorld;
	mat4 matViewProjection;
} variables;

layout (location = 0) in vec3 Vertex;
layout (location = 1) in vec3 Normal;

layout (location = 0) out vec3 vNormalWS;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
	vec4 vVertexWS = variables.matWorld * vec4(Vertex, 1.0);
	vNormalWS = normalize(Normal); // mat3(variables.matWorld) * Normal);
	gl_Position = vec4(0.1 * Vertex, 1.0); //variables.matViewProjection * vVertexWS;
}