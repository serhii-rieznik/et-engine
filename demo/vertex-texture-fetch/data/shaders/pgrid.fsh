uniform etLowp sampler2D cloudsTexture;

etFragmentIn etHighp vec3 vViewWS;
etFragmentIn etHighp vec3 vLightWS;
etFragmentIn etHighp vec3 vNormalWS;
etFragmentIn etHighp vec2 vTextureCoord0;

#include "cooktorrance.h"

void main()
{
	etHighp vec3 n = normalize(vNormalWS);
	etHighp vec3 l = normalize(vLightWS);
	etHighp vec3 v = normalize(vViewWS);

	etHighp vec4 c1 = etTexture2D(cloudsTexture, vTextureCoord0);

	etHighp float light = CookTorrance(n, l, v, 0.2);

	etHighp vec4 dc = vec4(1.0, 0.75, 0.5, 1.0);
	etHighp vec4 sc = vec4(0.0625, 0.125, 0.6666, 1.0);

	etFragmentOut = dc * c1.x * dot(n, l) + sc * light;
}