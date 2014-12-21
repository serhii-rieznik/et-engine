etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;
etVertexOut vec2 NormalizedTexCoord;

void main()
{
	NormalizedTexCoord = Vertex;
	TexCoord = 0.5 + 0.5 * NormalizedTexCoord;
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
