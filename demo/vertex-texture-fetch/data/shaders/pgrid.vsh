#define ENABLE_DISPLACE

uniform sampler2D cloudsTexture;

uniform mat4 mModelViewProjection;
uniform mat4 mInverseMVPMatrix;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform vec2 time;

etVertexIn vec2 Vertex;

etVertexOut vec3 vVertexWS;
etVertexOut vec3 vViewWS;
etVertexOut vec3 vLightWS;
etVertexOut vec3 vNormalWS;
etVertexOut vec2 vTextureCoord0;

const vec2 sideDir = vec2(1.0 / 1024.0, 0.0);
const vec2 frontDir = vec2(0.0, 1.0 / 1024.0);

float sampleHeight(in vec2 tc1, in float sideScale, in float heightScale)
{
	float h0 = etTexture2D(cloudsTexture, tc1).x;
	float h1 = etTexture2D(cloudsTexture, tc1 + sideScale * sideDir).x;
	float h2 = etTexture2D(cloudsTexture, tc1 - sideScale * sideDir).x;
	float h3 = etTexture2D(cloudsTexture, tc1 + sideScale * frontDir).x;
	float h4 = etTexture2D(cloudsTexture, tc1 - sideScale * frontDir).x;
	return heightScale * (h0 + h1 + h2 + h3 + h4) / 5.0;
}

vec3 calculateNormal(in vec2 tc1, in float centerHeight, in float scale, in float sideScale, in float heightScale)
{
	float px = sampleHeight(tc1 - sideDir, sideScale, heightScale);
	float nx = sampleHeight(tc1 + sideDir, sideScale, heightScale);
	
	float py = sampleHeight(tc1 - frontDir, sideScale, heightScale);
	float ny = sampleHeight(tc1 + frontDir, sideScale, heightScale);

	vec3 n1 = cross
	(
		vec3(sideScale * frontDir.x, scale * (centerHeight - py), sideScale * frontDir.y),
		vec3(sideScale * sideDir.x, scale * (px - centerHeight), sideScale * sideDir.y)
	);

	vec3 n2 = cross
	(
		vec3(sideScale * frontDir.x, scale * (ny - centerHeight), sideScale * frontDir.y),
		vec3(sideScale * sideDir.x, scale * (centerHeight - nx), sideScale * sideDir.y)
	 );
		
	return normalize(n1 + n2);
}

void main()
{
	vec4 start = mInverseMVPMatrix * vec4(Vertex, -1.0, 1.0);
	vec4 dir = mInverseMVPMatrix * vec4(Vertex, 1.0, 1.0) - start;
	vec4 vertex = start - (start.y / dir.y) * dir;
	
	vVertexWS = vertex.xyz / vertex.w;
	
	vTextureCoord0 = vec2(-0.005, 0.005) * vVertexWS.xz;
	
	float sideScale = 1.0 - exp(-0.25 * length(vCamera.xz - vVertexWS.xz) / vCamera.y);
	float heightScale = 10.0 * (1.0 - sideScale);
	
	vVertexWS.y = sampleHeight(vTextureCoord0, sideScale, heightScale);
	vNormalWS = calculateNormal(vTextureCoord0, vVertexWS.y, 0.005, sideScale, heightScale);
	
	vViewWS = vCamera - vVertexWS;
	vLightWS = vPrimaryLight - vVertexWS;

	gl_Position = mModelViewProjection * vec4(vVertexWS, 1.0);
}