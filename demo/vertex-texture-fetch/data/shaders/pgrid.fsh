uniform etLowp sampler2D cloudsTexture;

etFragmentIn etHighp vec3 vViewWS;
etFragmentIn etHighp vec3 vLightWS;
etFragmentIn etHighp vec3 vNormalWS;
etFragmentIn etHighp vec2 vTextureCoord0;
etFragmentIn etHighp vec2 vTextureCoord1;

#include "cooktorrance.h"

void main()
{
	etHighp vec3 n = normalize(vNormalWS);
	etHighp vec3 l = normalize(vLightWS);
	etHighp vec3 v = normalize(vViewWS);

	etHighp vec4 c1 = etTexture2D(cloudsTexture, vTextureCoord0);
	etHighp vec4 c2 = etTexture2D(cloudsTexture, vTextureCoord1);

	etHighp vec2 light = CookTorrance(n, l, v, 1.0 - c2.y);

	etHighp vec4 dc = vec4(1.0, 0.75, 0.5, 1.0);
	etHighp vec4 sc = vec4(0.0625, 0.125, 0.6666, 1.0);

	etFragmentOut = (0.5 + 0.5 * c1.x) * dc * light.x + sc * (c2.y + light.y);
}