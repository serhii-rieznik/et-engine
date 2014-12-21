uniform mat4 mModelViewProjection;
uniform mat4 mModelView;
uniform mat4 mTransform;

etVertexIn vec3 Vertex;
etVertexIn vec2 TexCoord0;
etVertexIn vec3 Normal;
etVertexIn vec3 Tangent;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vNormalWS;
etVertexOut vec3 vTBNr0;
etVertexOut vec3 vTBNr1;
etVertexOut vec3 vTBNr2;

void main()
{
	TexCoord = TexCoord0;
	
	mat3 mTransform3 = mat3(mModelView * mTransform);
	
	vec3 aTangent = normalize(mTransform3 * Tangent);
	vec3 aNormal = normalize(mTransform3 * Normal);
	vec3 aBiTangent = normalize(cross(aNormal, aTangent));
	
	vTBNr0 = vec3(aTangent.x, aBiTangent.x, aNormal.x);
	vTBNr1 = vec3(aTangent.y, aBiTangent.y, aNormal.y);
	vTBNr2 = vec3(aTangent.z, aBiTangent.z, aNormal.z);
	
	vNormalWS = aNormal;
	
	gl_Position = mModelViewProjection * mTransform * vec4(Vertex, 1.0);
}
