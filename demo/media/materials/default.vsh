uniform mat4 matWorld;
uniform mat4 matViewProjection;
uniform vec3 defaultCamera;
uniform vec3 defaultLight;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexOut vec3 vNormalWS;
etVertexOut vec3 vCameraDirectionWS;
etVertexOut vec3 vLightDirectionWS;

void main()
{
	vec4 vVertexWS = matWorld * vec4(Vertex, 1.0);
	vNormalWS = normalize(mat3(matWorld) * Normal);
    vCameraDirectionWS = defaultCamera - vVertexWS.xyz;
    vLightDirectionWS = defaultLight - vVertexWS.xyz;
	gl_Position = matViewProjection * vVertexWS;
}
