#version 450

layout (location = 0) in vec2 Vertex;
layout (location = 0) out vec2 TexCoord;

void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
