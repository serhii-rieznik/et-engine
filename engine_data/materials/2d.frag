#version 450

layout (binding = 0) uniform sampler2D color_texture;
layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = texture(color_texture, TexCoord);
}
