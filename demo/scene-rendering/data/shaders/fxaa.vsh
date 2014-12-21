uniform vec2 texel;

etVertexIn vec2 Vertex;

etVertexOut vec2 TexCoord;
etVertexOut vec2 TexCoordNW;
etVertexOut vec2 TexCoordNE;
etVertexOut vec2 TexCoordSW;
etVertexOut vec2 TexCoordSE;

void main()
{
	TexCoord = 0.5 + 0.5 * Vertex;
	
	TexCoordNW = TexCoord - texel;
	TexCoordNE = TexCoord + texel * vec2( 1.0, -1.0);
	TexCoordSW = TexCoord + texel * vec2(-1.0,  1.0);
	TexCoordSE = TexCoord + texel;
	
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
