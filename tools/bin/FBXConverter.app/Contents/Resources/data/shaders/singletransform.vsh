uniform mat4 mModelViewProjection;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform mat4 mTransform;

etVertexIn vec4 Vertex;
etVertexIn vec3 Normal;
etVertexIn vec2 TexCoord0;
etVertexIn vec4 Color;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec3 vNormalWS;
etVertexOut vec4 vColor;

void main()
{
	mat3 mTransform3 = mat3(mTransform);
	vec4 transVertex = mTransform * Vertex;

	vNormalWS = mTransform3 * Normal;
	vLightWS = vCamera - transVertex.xyz;
	vViewWS = vCamera - transVertex.xyz;
	
	vColor = vec4(1.0);//dot(Color.xyz, Color.xyz) < 0.01 ? vec4(1.0) : Color;

	TexCoord  = TexCoord0;
	gl_Position = mModelViewProjection * transVertex;
}