uniform mat4 mModelViewProjection;

etVertexIn vec3 Vertex;
etVertexIn vec4 Color;

etVertexOut etLowp vec4 aColor;

void main()
{
	aColor = Color;
	gl_Position = mModelViewProjection * vec4(Vertex, 1.0);
}