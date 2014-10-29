uniform etLowp sampler2D cloudsTexture;

etFragmentIn etHighp vec3 vViewWS;
etFragmentIn etHighp vec3 vLightWS;
etFragmentIn etHighp vec3 vNormalWS;
etFragmentIn etHighp vec2 vTextureCoord0;

#include "cooktorrance.h"

const etHighp vec4 diffuseColor = vec4(1.0, 0.75, 0.5, 1.0);
const etHighp vec4 specularColor = vec4(0.0625, 0.125, 0.6666, 1.0);

void main()
{
	etHighp vec3 n = normalize(vNormalWS);
	etHighp vec3 l = normalize(vLightWS);
	etHighp vec3 v = normalize(vViewWS);

	etHighp vec4 c1 = etTexture2D(cloudsTexture, vTextureCoord0);

	etHighp float diffuse = 0.5 + 0.5 * max(0.0, dot(n, l));
	etHighp float specular = CookTorrance(n, l, v, 0.72);

	etFragmentOut = diffuseColor * (c1.x * diffuse * diffuse) + specularColor * specular;
}