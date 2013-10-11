vec2 CookTorrance(vec3 _normal, vec3 _light, vec3 _view, float _roughness)
{
 float NdotL = max( dot( _normal, _light ), 0.0 );
 float Rs = 0.0;

 if (_roughness > 0.0)
  {
   vec3  half_vec = normalize( _view + _light );

   float NdotV = max( dot( _normal, _view ), 0.0 );
   float NdotH = max( dot( _normal, half_vec ), 1.0e-7 );
   float VdotH = max( dot( _view, half_vec ), 1.0e-7 );

   float geometric = 2.0 * NdotH / VdotH;
   geometric = min( 1.0, geometric * min(NdotV, NdotL) );

   float r_sq = _roughness * _roughness;
   float NdotH_sq = NdotH * NdotH;
   float NdotH_sq_r = 1.0 / (NdotH_sq * r_sq);
   float roughness_exp = (NdotH_sq - 1.0) * ( NdotH_sq_r );
   float roughness = 0.25 * exp(roughness_exp) * NdotH_sq_r / NdotH_sq;

   float fresnel = 1.0 / (1.0 + NdotV);

   Rs = fresnel * geometric * roughness / (NdotV + 1.0e-7);
  }

 return vec2(NdotL, min(1.0, Rs) );
}
