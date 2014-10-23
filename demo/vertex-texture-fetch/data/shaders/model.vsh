uniform mat4 mModelViewProjection;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform vec3 center;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;

etVertexOut vec3 vNormalWS;
etVertexOut vec3 vCameraWS;
etVertexOut vec3 vLightWS;

void main()
{
	vec3 pos = center + 15.0 * Vertex;
	
	vNormalWS = Normal;
	vCameraWS = vCamera - pos;
	vLightWS = vec3(0.0, 1.0, 0.0);
	
	gl_Position = mModelViewProjection * vec4(pos, 1.0);
}