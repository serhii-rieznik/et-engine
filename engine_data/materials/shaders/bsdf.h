struct Surface
{
	float roughness;
	float metallness;
	float roughnessSquared;
	float3 baseColor;
	float3 f0;
	float3 f90;
};

struct BSDF
{
	float LdotN;
	float VdotN;
	float LdotH;
	float NdotH;                                
};

Surface buildSurface(in float3 baseColor, in float m, in float r);
BSDF buildBSDF(in float3 n, in float3 l, in float3 v);

float ggxDistribution(in float NdotH, in float roughnessSquared);
float ggxMasking(in float VdotN, in float LdotN, in float roughnessSquared);
float diffuseBurley(in float LdotN, in float VdotN, in float LdotH, in float roughness);

float3 directDiffuse(in Surface surface, in BSDF bsdf);
float3 directSpecular(in Surface surface, in BSDF bsdf);
float3 directLighting(in Surface surface, in BSDF bsdf);

#define NORMALIZATION_SCALE 	INV_PI
#define MIN_ROUGHNESS 			0.0001
#define MIN_REFLECTANCE 		0.16
#define MIN_FLOAT				0.0000001

Surface buildSurface(in float3 baseColor, in float m, in float r)
{
	r *= r;
	float defaultReflectance = 0.5;
	float reflectance = MIN_REFLECTANCE * defaultReflectance * defaultReflectance;	
	                 	
	Surface	result;
	result.baseColor = baseColor * (1.0 - saturate(m));
	result.f90 = saturate(50.0 * reflectance);
	result.f0 = lerp(reflectance, baseColor, m);
	result.roughness = clamp(r, MIN_ROUGHNESS, 1.0);
	result.roughnessSquared = clamp(r * r, MIN_ROUGHNESS, 1.0);
	result.metallness = m;
	return result;
}

BSDF buildBSDF(in float3 n, in float3 l, in float3 v)
{                         
	float3 h  = normalize(l + v);

	BSDF bsdf;
	bsdf.LdotN = saturate(dot(l, n));
	bsdf.LdotH = saturate(dot(l, h));
	bsdf.NdotH = saturate(dot(n, h));
	bsdf.VdotN = max(dot(v, n), MIN_FLOAT);
	return bsdf;
}

float ggxDistribution(in float NdotH, in float roughnessSquared)
{
	float f = (NdotH * roughnessSquared - NdotH) * NdotH + 1.0;
	return roughnessSquared / max(PI * f * f, MIN_FLOAT);
}

float ggxMasking(in float VdotN, in float LdotN, in float roughnessSquared)
{
	float L1 = 0.5 * (sqrt(roughnessSquared + (1.0 - roughnessSquared) * VdotN * VdotN) / VdotN - 1.0); 
	float L2 = 0.5 * (sqrt(roughnessSquared + (1.0 - roughnessSquared) * LdotN * LdotN) / LdotN - 1.0); 
	return 1.0 / (1.0 + L1 + L2);
}

float ggxMaskingCombined(in float VdotN, in float LdotN, in float roughnessSquared)
{
	float VoN2 = VdotN * VdotN;
	float LoN2 = LdotN * LdotN;
	float L1 = LdotN * sqrt(roughnessSquared + (1.0 - roughnessSquared) * VoN2); 
	float L2 = VdotN * sqrt(roughnessSquared + (1.0 - roughnessSquared) * LoN2); 
	return 0.5 * LdotN / (L1 + L2);
}

float diffuseBurley(in float LdotN, in float VdotN, in float LdotH, in float roughness)
{
#define USE_FROSTBITE_VARIANT 1

	float Fl = pow(1.0 - LdotN, 5.0);
	float Fv = pow(1.0 - VdotN, 5.0);

#if (USE_FROSTBITE_VARIANT)
	float factor = lerp(1.0, 1.0 / 1.51, roughness);
	float fd90 = (0.5 + 2.0 * LdotH * LdotH) * roughness;
	float lightScatter = 1.0 + (fd90 - 1.0) * Fl;
	float viewScatter = 1.0 + (fd90 - 1.0) * Fv;
	return lightScatter * viewScatter * factor;
#else
	float Rr = 2.0 * roughness * LdotH * LdotH; 
	float Flambert = (1.0 - 0.5 * Fl) * (1.0 - 0.5 * Fv);
	float Fretroreflection = Rr * (Fl + Fv + Fl * Fv * (Rr - 1.0)); 
	return Flambert + Fretroreflection;
#endif
}

float3 computeDirectDiffuse(in Surface surface, in BSDF bsdf)
{
	float diffuseTerm = diffuseBurley(bsdf.LdotN, bsdf.VdotN, bsdf.LdotH, surface.roughness);
	return surface.baseColor * saturate(diffuseTerm * bsdf.LdotN * NORMALIZATION_SCALE);
}

float3 computeDirectSpecular(in Surface surface, in BSDF bsdf)
{
	float d = ggxDistribution(bsdf.NdotH, surface.roughnessSquared);
	float g = ggxMaskingCombined(bsdf.VdotN, bsdf.LdotN, surface.roughnessSquared);
	return g * d * lerp(surface.f0, surface.f90, pow(1.0 - bsdf.LdotH, 5.0));
}


