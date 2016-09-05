struct VSInput
{
	packed_float3 position [[attribute(0)]];
	packed_float3 normal [[attribute(1)]];
	packed_float2 texCoord [[attribute(2)]];
};

struct VSOutput
{
	float4 position [[position]];
	float3 normal;
	float3 cameraDirection;
	float3 lightDirection;
};

struct Uniforms
{
	float4x4 viewProjection;
	float4x4 transform;
};

vertex VSOutput vertexMain(constant VSInput* vsInput [[buffer(0)]],
						   constant Uniforms& uniforms [[buffer(1)]],
						   uint vertexId [[vertex_id]])
{
	VSOutput vOut;
	vOut.position = uniforms.viewProjection * uniforms.transform * float4(vsInput[vertexId].position, 1.0);
	vOut.normal = float3(0.0, 1.0, 0.0);
	vOut.cameraDirection = float3(0.0, 1.0, 0.0);
	vOut.lightDirection = float3(0.0, 1.0, 0.0);
	return vOut;
}

fragment float4 fragmentMain(VSOutput fragmentIn [[stage_in]])
{
	return float4(1.0, 0.5, 0.25, 1.0);
}

/*
#if defined(VERTEX_SHADER)

uniform mat4 matWorld;
uniform mat4 matViewProjection;
uniform vec3 defaultCamera;
uniform vec3 defaultLight;

etVertexIn vec3 Vertex;
etVertexIn vec3 Normal;
etVertexOut vec3 vNormalWS;
etVertexOut vec3 vCameraDirectionWS;
etVertexOut vec3 vLightDirectionWS;

void main()
{
	vec4 vVertexWS = matWorld * vec4(Vertex, 1.0);
	vNormalWS = normalize(mat3(matWorld) * Normal);
	vCameraDirectionWS = defaultCamera - vVertexWS.xyz;
	vLightDirectionWS = defaultLight;
	gl_Position = matViewProjection * vVertexWS;
}

#elif defined(FRAGMENT_SHADER)

uniform sampler2D diffuse_map;
uniform sampler2D opacity_map;
uniform vec4 diffuse_color;
uniform vec4 specular_color;
uniform float roughness;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec2 vTexCoord0;
etFragmentIn vec3 vCameraDirectionWS;
etFragmentIn vec3 vLightDirectionWS;

#include "microfacet.h"

void main()
{
    vec3 vNormal = normalize(vNormalWS);
    vec3 vView = normalize(vCameraDirectionWS);
    vec3 vLight = normalize(vLightDirectionWS);
    vec3 vHalf = normalize(vView + vLight);
    
    float NdotV = max(0.00001, dot(vNormal, vView));
    float NdotL = max(0.0, dot(vNormal, vLight));
	float HdotL = dot(vHalf, vLight);
    float NdotH = dot(vNormal, vHalf);

    float kS = microfacetSpecular(NdotL, NdotV, NdotH, roughness) * invPi;
    float kD = burleyDiffuse(NdotL, NdotV, HdotL, roughness) * invPi;
    
    etFragmentOut = diffuse_color * kD + specular_color * kS;
}

#endif
*/
