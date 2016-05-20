uniform sampler2D diffuse_map;
uniform sampler2D opacity_map;
uniform vec4 diffuse_color;
uniform vec4 specular_color;
uniform float roughness;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec2 vTexCoord0;
etFragmentIn vec3 vCameraDirectionWS;
etFragmentIn vec3 vLightDirectionWS;

#include "common.fsh"

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