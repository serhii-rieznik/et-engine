uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_mask;

etFragmentIn vec2 TexCoord;

etFragmentIn vec3 vNormalWS;
etFragmentIn vec3 vTBNr0;
etFragmentIn vec3 vTBNr1;
etFragmentIn vec3 vTBNr2;

void main()
{
	if (etTexture2D(texture_mask, TexCoord).x < 0.5) discard;
	
	vec3 sampledNormal = 2.0 * etTexture2D(texture_normal, TexCoord).xyz - 1.0;
	
	vec3 outNormal;
	outNormal.x = dot(sampledNormal, vTBNr0);
	outNormal.y = dot(sampledNormal, vTBNr1);
	outNormal.z = dot(sampledNormal, vTBNr2);
	
	etFragmentOut = etTexture2D(texture_diffuse, TexCoord);
	etFragmentOut1 = vec4(0.5 + 0.5 * outNormal, 1.0);
}
