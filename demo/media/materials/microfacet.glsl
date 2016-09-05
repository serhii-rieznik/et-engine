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
