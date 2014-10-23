uniform etLowp vec4 linesColor;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec3 vCameraWS;
etFragmentIn vec3 vLightWS;

#include <cooktorrance.h>

const float r = 0.174;

void main()
{
	vec3 v = normalize(vCameraWS);
	vec3 l = normalize(vLightWS);
	vec3 n = normalize(vNormalWS);
	
	float ct = CookTorrance(n, l, v, r);
	
	etFragmentOut = vec4(ct);
}