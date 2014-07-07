#define ENABLE_DISPLACE

uniform sampler2D cloudsTexture;

uniform mat4 mModelViewProjection;
uniform mat4 mInverseMVPMatrix;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform vec2 time;

etVertexIn vec2 Vertex;

etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec3 vNormalWS;
etVertexOut vec2 vTextureCoord0;
etVertexOut vec2 vTextureCoord1;

float sampleHeight(in vec2 tc1, in vec2 tc2)
{
	return etTexture2D(cloudsTexture, tc1).x + etTexture2D(cloudsTexture, tc2).y;
}

vec3 calculateNormal(in vec2 tc1, in vec2 tc2, in float hc, in float scale)
{
	vec2 sideDir = vec2(0.001, 0.0);
	vec2 frontDir = vec2(0.0, 0.001);
	
	float hpx = sampleHeight(tc1 - sideDir, tc2 - sideDir);
	float hpy = sampleHeight(tc1 - frontDir, tc2 - frontDir);
	float hnx = sampleHeight(tc1 + sideDir, tc2 + sideDir);
	float hny = sampleHeight(tc1 + frontDir, tc2 + frontDir);

	vec3 n1 = cross(vec3(frontDir.x, scale * (hpy - hc), frontDir.y),
		vec3(sideDir.x, scale * (hc - hpx), sideDir.y));

	vec3 n2 = cross(vec3(-frontDir.x, scale * (hc - hny), -frontDir.y),
		vec3(-sideDir.x, scale * (hc - hnx), -sideDir.y));
		
	return normalize(vec3(n2.x, (n1.y + n2.y), n1.z));
}

void main()
{
	vec4 start = mInverseMVPMatrix * vec4(Vertex, -1.0, 1.0);
	vec4 dir = mInverseMVPMatrix * vec4(Vertex, 1.0, 1.0) - start;
	vec4 worldPos = start - (start.y / dir.y) * dir;
	
	worldPos /= worldPos.w;

	vTextureCoord1 = vec2(0.01, -0.01) * worldPos.xz;
	vTextureCoord0 = vTextureCoord1 + vec2(time);

	float hc = sampleHeight(vTextureCoord0, vTextureCoord1);
	float heightAspect = normalize(vCamera - worldPos.xyz).y;
	worldPos.y += 10.0 * (heightAspect * heightAspect) * hc;

	vNormalWS = calculateNormal(vTextureCoord0, vTextureCoord1, hc, 0.05 * heightAspect);
	vViewWS = vCamera - worldPos.xyz;
	vLightWS = vPrimaryLight - worldPos.xyz;

	gl_Position = mModelViewProjection * worldPos;
}