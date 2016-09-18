/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/bsdf.h>

et::rt::BSDFSample::BSDFSample(const et::rt::float4& _wi, const et::rt::float4& _n,
	const Material& mat, Direction _d) : Wi(_wi), n(_n), IdotN(_wi.dot(_n)), dir(_d), alpha(mat.roughness)
{
	switch (mat.cls)
	{
		case Material::Class::Diffuse:
		{
			cls = BSDFSample::Class::Diffuse;
			Wo = randomVectorOnHemisphere(n, et::rt::cosineDistribution);
			color = mat.diffuse;
			break;
		}

		case Material::Class::Conductor:
		{
			cls = BSDFSample::Class::Reflection;
			Wo = computeReflectionVector(Wi, n, mat.roughness);
			fresnel = 0.98f;
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
					Wo = computeReflectionVector(Wi, n, mat.roughness);
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Transmittance;
					color = mat.diffuse;
					Wo = computeRefractionVector(Wi, n, eta, mat.roughness, IdotN, std::sqrt(refractionK) * signNoZero(IdotN));
				}
			}
			else // non-refractive material
			{
				fresnel = fresnelShlickApproximation(IdotN, eta);
				if (fastRandomFloat() <= fresnel)
				{
					cls = BSDFSample::Class::Reflection;
					Wo = computeReflectionVector(Wi, n, mat.roughness);
					color = mat.specular;
				}
				else
				{
					cls = BSDFSample::Class::Diffuse;
					Wo = computeDiffuseVector(Wi, n, mat.roughness);
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
	const Material& mat, Direction _d)
	: Wi(_wi)
	, Wo(_wo)
	, n(_n)
	, IdotN(_wi.dot(_n))
	, OdotN(_wo.dot(_n))
	, dir(_d)
	, alpha(mat.roughness)
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
			fresnel = 0.98f;
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

inline float G_ggx(float t, float rSq)
{
	t *= t;
	return 2.0f / (1.0f + std::hypot(1.0f, rSq * (1.0f - t) / t));
}

inline float D_ggx(float rSq, float ct)
{
	return rSq / (PI * et::sqr(ct * ct * (rSq - 1.0f) + 1.0f));
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
		float rSq = sqr(alpha);
		float ndf = D_ggx(rSq, NdotH);
		float g1 = G_ggx(NdotI, rSq) * float(HdotI / NdotI > 0.0f);
		float g2 = G_ggx(NdotO, rSq) * float(HdotO / NdotO > 0.0f);
		float g = g1 * g2;
		return (ndf * g * fresnel) / (4.0f * NdotI * NdotO);
	}
	else if (cls == BSDFSample::Class::Transmittance)
	{
		float HdotO = h.dot(Wo);
		float HdotI = h.dot(Wi);
		float rSq = sqr(alpha);

		float g1 = G_ggx(IdotN, rSq) * float(HdotI / IdotN > 0.0f);
		float g2 = G_ggx(OdotN, rSq) * float(HdotO / OdotN > 0.0f);
		float g = g1 * g2;

		float NdotH = n.dot(h);
		float etaSq = eta * eta;
		float d = D_ggx(rSq, NdotH) * float(NdotH > 0.0f);
		float denom = sqr(HdotI + eta * HdotO);

		return ((1 - fresnel) * d * g * etaSq * HdotI * HdotO) / (IdotN * denom);
	}

	ET_FAIL("Invalid material class");
	return 0.0f;
}

float et::rt::BSDFSample::pdf()
{
	if (cls == Class::Diffuse)
		return cosTheta / PI;

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
	float pdfValue = pdf();

	if (pdfValue == 0.0f)
		return float4(0.0f);

	return color * (cosTheta * bsdf() / pdfValue);
}
