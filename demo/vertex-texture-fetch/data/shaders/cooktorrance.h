float CookTorrance(vec3 n, vec3 l, vec3 v, float r)
{
	float NdotL = dot(n, l);
	if (NdotL > 0.0)
	{
		vec3 h = normalize(v + l);
		
		float NdotV = dot(n, v);
		float NdotH = dot(n, h);
		float VdotH = dot(v, h);
		float NdotH2 = NdotH * NdotH;
		float NdotH2r2 = NdotH2 * r * r;
		float roughness_exp = (NdotH2 - 1.0) / NdotH2r2;
		float roughness = 0.25 * exp(roughness_exp) / (NdotH2r2 * NdotH2);
		float fresnel = 1.0 / (NdotV + 1.0);
		float geometric = min(1.0, 2.0 * NdotH * min(NdotV, NdotL) / VdotH);
		return geometric * roughness * fresnel / NdotV;
	}
	return 0.0;
}
