/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/bsdf.h>

#define ET_RT_USE_COSINE_WEIGHTED_SAMPLING 0

et::rt::BSDFSample::BSDFSample(const et::rt::float4& _wi, const et::rt::float4& _n, const Material& mat,
	const et::rt::float4& uv, Direction _d) : Wi(_wi), n(_n), IdotN(_wi.dot(_n)), alpha(mat.roughness), dir(_d)
{
	switch (mat.cls)
	{
		case Material::Class::Diffuse:
		{
			cls = BSDFSample::Class::Diffuse;
#		if (ET_RT_USE_COSINE_WEIGHTED_SAMPLING)
			Wo = randomVectorOnHemisphere(n, et::rt::cosineDistribution);
#		else
			Wo = randomVectorOnHemisphere(n, et::rt::uniformDistribution);
#		endif
			color = mat.diffuse;
			break;
		}

		case Material::Class::Conductor:
		{
			cls = BSDFSample::Class::Reflection;
			Wo = computeReflectionVector(Wi, n, alpha);
			fresnel = fresnelShlickApproximation(IdotN, 0.025f);
			color = mat.specular;
			break;
		}

		case Material::Class::Dielectric:
		{
			eta = mat.ior;
			if (eta > 1.0f) // refractive
			{
				if (IdotN < 0.0f)
				{
					eta = 1.0f / eta;
				}
				else
				{
					n *= -1.0f;
					IdotN = -IdotN;
				}

				float sinTheta = 1.0f - sqr(eta) * (1.0f - sqr(IdotN));
				fresnel = (sinTheta > 0.0f) ? fresnelShlickApproximation(IdotN, eta) : 1.0f;
				if (fastRandomFloat() <= fresnel)
				{
					cls = BSDFSample::Class::Reflection;
					Wo = computeReflectionVector(Wi, n, alpha);
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Transmittance;
					Wo = computeRefractionVector(Wi, n, eta, alpha, sinTheta, IdotN);
					color = mat.diffuse;
				}
			}
			else // non-refractive material
			{
				fresnel = fresnelShlickApproximation(IdotN, eta);
				if (fastRandomFloat() <= fresnel)
				{
					cls = BSDFSample::Class::Reflection;
					Wo = computeReflectionVector(Wi, n, alpha);
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Diffuse;
					Wo = computeDiffuseVector(Wi, n, alpha);
					color = mat.diffuse;
				}
			}
			break;
		}

		default:
		ET_FAIL("Invalid material class");
	}

#if ET_RT_VISUALIZE_BRDF
	Wo = float4(0.0f, 1.0f, 0.0f, 0.0f);
#endif

	OdotN = Wo.dot(n);
	cosTheta = std::abs((dir == BSDFSample::Direction::Backward ? OdotN : IdotN));
}

et::rt::BSDFSample::BSDFSample(const et::rt::float4& _wi, const et::rt::float4& _wo, const et::rt::float4& _n,
	const Material& mat, const et::rt::float4& uv, Direction _d)
	: Wi(_wi)
	, Wo(_wo)
	, n(_n)
	, IdotN(_wi.dot(_n))
	, OdotN(_wo.dot(_n))
	, alpha(mat.roughness)
	, dir(_d)
{
	switch (mat.cls)
	{
		case Material::Class::Diffuse:
		{
			cls = BSDFSample::Class::Diffuse;
			color = mat.diffuse;
			break;
		}

		case Material::Class::Conductor:
		{
			cls = BSDFSample::Class::Reflection;
			fresnel = fresnelShlickApproximation(IdotN, 0.025f);
			color = mat.specular;
			break;
		}

		case Material::Class::Dielectric:
		{
			eta = mat.ior;
			if (eta > 1.0f) // refractive
			{
				if (IdotN < 0.0f)
					eta = 1.0f / eta;

				float refractionK = 1.0f - sqr(eta) * (1.0f - sqr(IdotN));
				fresnel = (refractionK > 0.0f) ? fresnelShlickApproximation(IdotN, eta) : 1.0f;
				if (fastRandomFloat() <= fresnel)
				{
					cls = BSDFSample::Class::Reflection;
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Transmittance;
					color = mat.diffuse;
				}
			}
			else // non-refractive material
			{
				fresnel = fresnelShlickApproximation(IdotN, eta);
				if (fastRandomFloat() <= fresnel)
				{
					cls = BSDFSample::Class::Reflection;
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Diffuse;
					color = mat.diffuse;
				}
			}
			break;
		}

		default:
			ET_FAIL("Invalid material class");
	}

	cosTheta = std::abs((dir == BSDFSample::Direction::Backward ? OdotN : IdotN));
}

inline float G_ggx(float t, float alpha)
{
	float tanThetaSquared = (1.0f - t * t) / (t * t);
	return 2.0f / (1.0f + std::sqrt(1.0f + alpha * alpha * tanThetaSquared));
}

inline float D_ggx(float alphaSquared, float cosTheta)
{
	union { float f; uint32_t i; } x = { cosTheta * cosTheta * (alphaSquared - 1.0f) + 1.0f };
	return (x.i & 0x7fffffff) ? (alphaSquared / (PI * et::sqr(x.f))) : 1.0f;
}

float et::rt::BSDFSample::bsdf()
{
	if (cls == Class::Diffuse)
		return 1.0f / PI;

	if (cls == Class::Reflection)
	{
		ET_ASSERT(h.dotSelf() > 0.0f);
		float NdotO = n.dot(Wo);
		float NdotI = -n.dot(Wi);
		float HdotO = h.dot(Wo);
		float HdotI = -h.dot(Wi);
		float NdotH = n.dot(h);
		float ndf = D_ggx(alpha * alpha, NdotH);
		float g1 = G_ggx(NdotI, alpha) * float(HdotI / NdotI > 0.0f);
		float g2 = G_ggx(NdotO, alpha) * float(HdotO / NdotO > 0.0f);
		float g = g1 * g2;
		return (ndf * g * fresnel) / (4.0f * NdotI * NdotO);
	}
	else if (cls == BSDFSample::Class::Transmittance)
	{
		float HdotO = h.dot(Wo);
		float HdotI = h.dot(Wi);
		float g1 = G_ggx(IdotN, alpha) * float(HdotI / IdotN > 0.0f);
		float g2 = G_ggx(OdotN, alpha) * float(HdotO / OdotN > 0.0f);
		float g = g1 * g2;
		float NdotH = n.dot(h);
		float etaSq = eta * eta;
		float d = D_ggx(alpha * alpha, NdotH) * float(NdotH > 0.0f);
		float denom = sqr(HdotI + eta * HdotO);

		return ((1.0f - fresnel) * d * g * etaSq * HdotI * HdotO) / (IdotN * denom + Constants::epsilon);
	}

	ET_FAIL("Invalid material class");
	return 0.0f;
}

float et::rt::BSDFSample::pdf()
{
	if (cls == Class::Diffuse)
	{
#	if (ET_RT_USE_COSINE_WEIGHTED_SAMPLING)
		return cosTheta / PI;
#	else
		return 1.0f / DOUBLE_PI;
#	endif
	}

	if (cls == Class::Reflection)
	{
		h = Wo - Wi;
		h.normalize();

		float HdotI = -h.dot(Wi);
		float NdotH = n.dot(h);
		float ndf = D_ggx(alpha * alpha, NdotH) * float(NdotH > 0.0f);
		return (ndf * NdotH) / (4.0f * HdotI);
	}
	else if (cls == BSDFSample::Class::Transmittance)
	{
		h = Wi + Wo * eta;
		h.normalize();
		h *= 2.0f * float(h.dot(n) > 0.0f) - 1.0f;

		float HdotO = h.dot(Wo);
		float HdotI = h.dot(Wi);
		float rSq = sqr(alpha);

		float NdotH = n.dot(h);
		float etaSq = eta * eta;
		float d = D_ggx(rSq, NdotH) * float(NdotH > 0.0f);
		return d * etaSq * HdotO / sqr(HdotI + eta * HdotO);
	}

	ET_FAIL("Invalid material class");
	return 0.0f;
}

et::rt::float4 et::rt::BSDFSample::evaluate()
{
	union { float f; uint32_t i; } x = { pdf() };
	return (x.i & 0x7fffffff) ? color * (cosTheta * bsdf() / x.f) : float4(0.0f);
}

et::rt::float4 et::rt::BSDFSample::combinedEvaluate()
{
	if (cls == Class::Diffuse)
	{
#	if (ET_RT_USE_COSINE_WEIGHTED_SAMPLING)
		return color;
#	else
		return color * (2.0f * cosTheta);
#	endif
	}

	if (cls == Class::Reflection)
	{
		h = Wo - Wi;
		h.normalize();
		ET_ASSERT(h.dotSelf() > 0.0f);
		float HdotO = h.dot(Wo);
		float NdotH = n.dot(h);
		float HdotIdivIdotN = h.dot(Wi) / IdotN;
		float g1 = G_ggx(-IdotN, alpha) * float(HdotIdivIdotN > 0.0f);
		float g2 = G_ggx( OdotN, alpha) * float(HdotO / OdotN > 0.0f);
		return color * (g1 * g2 * fresnel * HdotIdivIdotN / NdotH);
	}

	if (cls == BSDFSample::Class::Transmittance)
	{
		h = Wi + Wo * eta;
		h.normalize();
		h *= 2.0f * float(h.dot(n) > 0.0f) - 1.0f;
		float HdotO = h.dot(Wo);
		float HdotI = h.dot(Wi);
		float HdotIdivIdotN = HdotI / IdotN;
		float g1 = G_ggx(IdotN, alpha) * float(HdotIdivIdotN > 0.0f);
		float g2 = G_ggx(OdotN, alpha) * float(HdotO / OdotN > 0.0f);
		return color * (cosTheta * (1.0f - fresnel) * g1 * g2 * HdotIdivIdotN * HdotO / HdotO);
	}

	ET_FAIL("Invalid material class");
	return float4(0.0f);
}

