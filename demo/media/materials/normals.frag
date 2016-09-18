#version 450

layout (location = 0) in vec3 vNormalWS;
layout (location = 0) out vec4 fragmentOut;

void main()
{
    vec3 vNormal = normalize(vNormalWS);
    fragmentOut = vec4(0.5 + 0.5 * vNormal, 1.0);
}

