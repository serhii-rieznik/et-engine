uniform vec2 direction;

etVertexIn vec2 Vertex;

#define NUM_SAMPLES	4

etVertexOut vec2 CenterTexCoord;
etVertexOut vec3 PreviousTexCoords[NUM_SAMPLES];
etVertexOut vec3 NextTexCoords[NUM_SAMPLES];

void main()
{
	float dw = 0.225;
	
	CenterTexCoord = 0.5 + 0.5 * Vertex;
	
	NextTexCoords[0] = vec3(CenterTexCoord + direction, dw);
	PreviousTexCoords[0] = vec3(CenterTexCoord - direction, dw);
	
	for (int i = 1; i < NUM_SAMPLES; ++i)
	{
		NextTexCoords[i] = NextTexCoords[i-1] + vec3(direction, dw);
		PreviousTexCoords[i] = PreviousTexCoords[i-1] - vec3(direction, dw);
	}
/*
	PreviousTexCoords[2] = PreviousTexCoords[1] - vec3(direction, dw);
	PreviousTexCoords[3] = PreviousTexCoords[2] - vec3(direction, dw);
	NextTexCoords[2] = NextTexCoords[1] + vec3(direction, dw);
	NextTexCoords[3] = NextTexCoords[2] + vec3(direction, dw);
*/
	
	gl_Position = vec4(Vertex, 0.0, 1.0);
}
