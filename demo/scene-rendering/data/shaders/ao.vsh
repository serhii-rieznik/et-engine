uniform vec2 noiseTextureScale;

etVertexIn vec2 Vertex;

etVertexOut vec2 NormalizedTexCoord;
etVertexOut vec2 TexCoord;
etVertexOut vec2 NoiseTexCoord;

void main()
{
	NormalizedTexCoord = Vertex;
	TexCoord = 0.5 + 0.5 * NormalizedTexCoord;
	NoiseTexCoord = noiseTextureScale * TexCoord;
	
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
