etHighp vec2 CookTorrance(etHighp  vec3 _normal, etHighp vec3 _light, etHighp vec3 _view, etHighp float _roughness)
{
	etHighp float NdotL = max( dot( _normal, _light ), 0.0 );
	etHighp float Rs = 0.0;
	
	if (_roughness > 0.0)
	{
		etHighp vec3 half_vec = normalize( _view + _light );
		
		etHighp float NdotV = max( dot( _normal, _view ), 0.0 );
		etHighp float NdotH = max( dot( _normal, half_vec ), 1.0e-7 );
		etHighp float VdotH = max( dot( _view, half_vec ), 1.0e-7 );
		
		etHighp float geometric = 2.0 * NdotH / VdotH;
		geometric = min( 1.0, geometric * min(NdotV, NdotL) );
		
		etHighp float r_sq = _roughness * _roughness;
		etHighp float NdotH_sq = NdotH * NdotH;
		etHighp float NdotH_sq_r = 1.0 / (NdotH_sq * r_sq);
		etHighp float roughness_exp = (NdotH_sq - 1.0) * ( NdotH_sq_r );
		etHighp float roughness = 0.25 * exp(roughness_exp) * NdotH_sq_r / NdotH_sq;
		etHighp float fresnel = 1.0 / (1.0 + NdotV);
		
		Rs = fresnel * geometric * roughness / (NdotV + 1.0e-7);
	}
	
	return vec2(NdotL, min(1.0, Rs));
}
