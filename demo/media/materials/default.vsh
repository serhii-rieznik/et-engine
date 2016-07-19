uniform mat4 matWorld;
uniform mat4 matViewProjection;
uniform vec3 defaultCamera;
uniform vec3 defaultLight;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;

etVertexOut vec3 vNormalWS;

#if defined(MICROFACET_MATERIAL)
etVertexOut vec3 vCameraDirectionWS;
etVertexOut vec3 vLightDirectionWS;
#endif

void main()
{
	vec4 vVertexWS = matWorld * vec4(Vertex, 1.0);
	vNormalWS = normalize(mat3(matWorld) * Normal);

#if defined(MICROFACET_MATERIAL)
    vCameraDirectionWS = defaultCamera - vVertexWS.xyz;
    vLightDirectionWS = defaultLight;
#endif

	gl_Position = matViewProjection * vVertexWS;
}
