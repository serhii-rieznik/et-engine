#if defined(VERTEX_SHADER)

uniform mat4 matWorld;
uniform mat4 matViewProjection;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexOut vec3 vNormalWS;

void main()
{
	vec4 vVertexWS = matWorld * vec4(Vertex, 1.0);
	vNormalWS = normalize(mat3(matWorld) * Normal);
	gl_Position = matViewProjection * vVertexWS;
}

#elif defined(FRAGMENT_SHADER)

etFragmentIn vec3 vNormalWS;

void main()
{
    vec3 vNormal = normalize(vNormalWS);
    etFragmentOut = vec4(0.5 + 0.5 * vNormal, 1.0);
}

#endif
