uniform vec2 scale;
uniform vec2 offset;

etVertexIn vec2 Vertex;
etVertexOut vec2 vertex;

void main()
{
	vertex = offset + scale * Vertex;
	gl_Position = vec4(vertex, 0.0, 1.0);
}