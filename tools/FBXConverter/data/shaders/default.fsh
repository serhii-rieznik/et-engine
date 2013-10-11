uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;

uniform vec4 ambientColor;
uniform vec4 diffuseColor;
uniform vec4 specularColor;
uniform float roughness;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec3 vViewWS;
etFragmentIn vec3 vLightWS;
etFragmentIn vec3 vViewTS;
etFragmentIn vec3 vLightTS;
etFragmentIn vec2 TexCoord;
etFragmentIn vec4 vColor;

#include <include/Phong.h>

vec2 defaultLight()
{
	vec3 nl = normalize(vLightWS);
	vec3 nv = normalize(vViewWS);
	vec3 nn = normalize(vNormalWS);
	return Phong(nn, nl, nv, 4.0);
}

void main()
{
	vec4 diffuse = etTexture2D(diffuseMap, TexCoord);

	if (dot(diffuse, diffuse) == 0.0)
		diffuse = diffuseColor;
		
	if (diffuse.w < 0.5) discard;

	vec2 light = defaultLight();
	light.x = 0.5 + 0.5 * light.x;

	etFragmentOut = (vColor * diffuse) * vec4(light.x * light.x) + (diffuse.w * light.y) * specularColor;
	etFragmentOut.w = diffuse.w;
}