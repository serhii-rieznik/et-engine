struct Surface
{
	float roughness;
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

/*
 *
 * Implementation
 *
 */
                                                                                                     
Surface buildSurface(in float3 baseColor, in float m, in float r)
{
	float defaultReflectance = 0.5;
	float reflectance = MIN_REFLECTANCE * defaultReflectance * defaultReflectance;	
	                 	
	Surface	result;
	result.baseColor = baseColor * (1.0 - saturate(m));
	result.f90 = saturate(50.0 * reflectance);
	result.f0 = lerp(reflectance, baseColor, m);
	result.roughness = clamp(r * r, MIN_ROUGHNESS, 1.0);
	result.roughnessSquared = result.roughness * result.roughness;
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
	float VdotN2 = VdotN * VdotN;
	float LdotN2 = LdotN * LdotN;
	float g1 = 2.0 / (1.0 + sqrt(1.0 + roughnessSquared * (1.0 - VdotN2) / VdotN2));
	float g2 = 2.0 / (1.0 + sqrt(1.0 + roughnessSquared * (1.0 - LdotN2) / LdotN2));
	return g1 * g2;
}

float ggxMaskingCombined(in float VdotN, in float LdotN, in float roughnessSquared)
{
	float a = roughnessSquared;
	float VdotN2 = VdotN * VdotN;
	float LdotN2 = LdotN * LdotN;
	return 1.0 / ((VdotN + sqrt(VdotN2 * (1.0 - a) + a)) * (1.0 + sqrt(1.0 - a + a / (LdotN2 + MIN_FLOAT))));
}

float diffuseBurley(in float LdotN, in float VdotN, in float LdotH, in float roughness)
{
#define USE_FROSTBINE_VARIANT 1

	float Fl = pow(1.0 - LdotN, 5.0);
	float Fv = pow(1.0 - VdotN, 5.0);

#if (USE_FROSTBINE_VARIANT)
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

float3 directLighting(in Surface surface, in BSDF bsdf)
{
	float diffuseTerm = diffuseBurley(bsdf.LdotN, bsdf.VdotN, bsdf.LdotH, surface.roughness);
	float3 diffuse = surface.baseColor * saturate(diffuseTerm * bsdf.LdotN * NORMALIZATION_SCALE);

	float d = ggxDistribution(bsdf.NdotH, surface.roughnessSquared);
	float g = ggxMasking(bsdf.VdotN, bsdf.LdotN, surface.roughnessSquared);
	float3 specular = d * g * lerp(surface.f0, surface.f90, pow(1.0 - bsdf.LdotH, 5.0));

	return 0.0 * diffuse + specular * float3(1.0, 0.0, 0.0);
}


