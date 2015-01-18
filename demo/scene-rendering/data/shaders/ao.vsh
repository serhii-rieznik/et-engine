uniform vec2 noiseTextureScale;

etVertexIn vec2 Vertex;

etVertexOut vec2 TexCoord;
etVertexOut vec2 NoiseTexCoord;

void main()
{
	TexCoord = 0.5 + 0.5 * Vertex;
	NoiseTexCoord = noiseTextureScale * TexCoord;
	
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
