//
//  Raytracer.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <stack>
#include <emmintrin.h>

#include <et/collision/collision.h>
#include "Raytracer.h"

using namespace rt;
using namespace et;

#if (ET_ENABLE_VEC4_ALIGN == 0)
#	error Compile with ET_ENABLE_VEC4_ALIGN enabled
#endif

typedef std::deque<float, et::SharedBlockAllocatorSTDProxy<float>> IndexOfRefractionDeque;

vec3 randomDiffuseVector(const vec3& normal);
vec3 randomReflectedVector(const vec3& incidence, const vec3& normal, float roughness, vec3& idealReflection);
vec3 randomRefractedVector(const vec3& incidence, const vec3& normal, float eta, float k, float roughness, vec3& idealRefraction);
vec3 refract(const vec3& incidence, const vec3& normal, float eta, float k);

float calculateRefractiveCoefficient(const vec3& incidence, const vec3& normal, float eta);
float computeFresnelTerm(const vec3& incidence, const vec3& normal, float indexOfRefraction);

vec4 gatherBouncesRecursive(const RaytraceScene& scene, const ray3d& ray, size_t depth, size_t maxDepth,
	IndexOfRefractionDeque& mediumIORs, const vec4& terminatingColor);

vec4 computeReflection(const RaytraceScene& scene, const SceneMaterial& mat, const vec3& rayDirection,
	const vec3& startPoint, const vec3& startNormal, size_t depth, size_t maxDepth,
	IndexOfRefractionDeque& mediumIORs, const vec4& terminatingColor);

vec4 performRaytracing(const RaytraceScene& scene, const ray3d& ray);
vec4 sampleEnvironmentColor(const RaytraceScene& scene, const ray3d& r);

inline vec4 add(const vec4& l, const vec4& r)
{
	vec4 result;
	_mm_store_ps(result.c, _mm_add_ps(_mm_load_ps(l.c), _mm_load_ps(r.c)));
	return result;
}

inline void mul(vec4& l, float s)
{
	_mm_store_ps(l.c, _mm_mul_ps(_mm_load_ps(l.c), _mm_set_ps1(s)));
}

inline vec4 mul(const vec4& l, const vec4& r)
{	
	vec4 result;
	_mm_store_ps(result.c, _mm_mul_ps(_mm_load_ps(l.c), _mm_load_ps(r.c)));
	return result;
}

inline vec4 mixi(const vec4& l, const vec4& r, float t)
{
	auto il = _mm_load_ps(l.c);
	auto ir = _mm_load_ps(r.c);
	
	vec4 result;
	_mm_store_ps(result.c, _mm_add_ps(il, _mm_mul_ps(_mm_set_ps1(t), _mm_sub_ps(ir, il))));
	return result;
}

inline vec2 subi2(const vec2& l, const vec2& r)
{
	vec4 result;
	_mm_storeu_ps(result.c, _mm_sub_ps(_mm_set_ps(0.0f, 0.0f, l.y, l.x), _mm_set_ps(0.0f, 0.0f, r.y, r.x)));
	return result.xy();
}

inline vec2 floori2(const vec2& v)
{
	static const __m128 one = _mm_set_ps1(1.0f);
	
	__m128 a = _mm_set_ps(0.0f, 0.0f, v.y, v.x);
	__m128 fval = _mm_cvtepi32_ps(_mm_cvttps_epi32(a));
	
	vec4 result;
	_mm_store_ps(result.c, _mm_sub_ps(fval, _mm_and_ps(_mm_cmplt_ps(a, fval), one)));
	return result.xy();
}

inline vec2 fracti2(const vec2& v)
{
	vec4 result;
	__m128 a = _mm_set_ps(0.0f, 0.0f, v.y, v.x);
	_mm_store_ps(result.c, _mm_sub_ps(a, _mm_cvtepi32_ps(_mm_cvttps_epi32(a))));
	return result.xy();
}

inline float randomFloatFrom1to1()
{
	static float values[512] = { };
	static bool shouldInitialize = true;
	if (shouldInitialize)
	{
		for (int i = 0; i < 512; ++i)
			values[i] = randomFloat(-1.0f, 1.0f);
		shouldInitialize = false;
	}
	return values[rand() % 512];
}

vec4 sampleTexture(const TextureDescription::Pointer& tex, vec2i texCoord)
{
	{
		while (texCoord.x >= tex->size.x) texCoord.x -= tex->size.x;
		while (texCoord.y >= tex->size.y) texCoord.y -= tex->size.y;
		while (texCoord.x < 0) texCoord.x += tex->size.x;
		while (texCoord.y < 0) texCoord.y += tex->size.y;
	}
	
	const vec4* rawData = reinterpret_cast<const vec4*>(tex->data.binary());
	return rawData[texCoord.x + texCoord.y * tex->size.x];
}

vec4 sampleEnvironmentColor(const RaytraceScene& scene, const ray3d& r)
{
	if (scene.environmentMap.invalid())
		return scene.ambientColor;
	
	ET_ASSERT(scene.environmentMap->bitsPerPixel == 128)
	
	float phi = 0.5f + std::atan2(r.direction.z, r.direction.x) / DOUBLE_PI;
	float theta = 0.5f + std::asin(r.direction.y) / PI;
	
	vec2 tc(phi * scene.environmentMap->size.x, theta * scene.environmentMap->size.y);
	vec2i baseTexCoord(static_cast<int>(tc.x), static_cast<int>(tc.y));
	
	vec4 c00 = sampleTexture(scene.environmentMap, baseTexCoord); ++baseTexCoord.x;
	vec4 c10 = sampleTexture(scene.environmentMap, baseTexCoord); ++baseTexCoord.y;
	vec4 c11 = sampleTexture(scene.environmentMap, baseTexCoord); --baseTexCoord.x;
	vec4 c01 = sampleTexture(scene.environmentMap, baseTexCoord);
	
	vec2 dudv = fracti2(tc);

	return mul(scene.ambientColor, mixi(mixi(c00, c10, dudv.x), mixi(c01, c11, dudv.x), dudv.y));
}

vec4 computeReflection(const RaytraceScene& scene, const SceneMaterial& mat, const vec3& rayDirection,
	const vec3& startPoint, const vec3& startNormal, size_t depth, size_t maxDepth, IndexOfRefractionDeque& mediumIORs,
	const vec4& terminatingColor)
{
	vec3 ideal;
	vec3 reflectedRay = randomReflectedVector(rayDirection, startNormal, mat.roughness, ideal);
	auto deepBounce = gatherBouncesRecursive(scene, ray3d(startPoint, reflectedRay), depth, maxDepth, mediumIORs, terminatingColor);
	mul(deepBounce, dot(reflectedRay, ideal));
	return add(mat.emissiveColor, mul(mat.reflectiveColor, deepBounce));
}

vec4 gatherBouncesRecursive(const RaytraceScene& scene, const ray3d& ray, size_t depth, size_t maxDepth,
	IndexOfRefractionDeque& mediumIORs, const vec4& terminatingColor)
{
	if (depth >= maxDepth)
		return terminatingColor;
	
	auto i = scene.findNearestIntersection(ray);
	
	if (!i.objectHit)
		return sampleEnvironmentColor(scene, ray);
	
	SceneMaterial mat = scene.materialAtIndex(i.materialIndex);
	
	if (mat.refractiveIndex > 0.0f)
	{
		bool enteringMaterial = dot(ray.direction, i.hitNormal) < 0.0f;
		bool hasNonDefaultIOR = (mediumIORs.size() > 1);
		
		float currentIOR = 1.0f;
		float targetIOR = 1.0f;
		
		if (enteringMaterial)
		{
			currentIOR = mediumIORs.back();
			targetIOR = mat.refractiveIndex;
		}
		else
		{
			currentIOR = mat.refractiveIndex;
			targetIOR = hasNonDefaultIOR ? *(mediumIORs.crbegin() + 1) : mediumIORs.back();
			i.hitNormal *= -1.0f;
		}
		
		float eta = currentIOR / targetIOR;
		float k = calculateRefractiveCoefficient(ray.direction, i.hitNormal, eta);
		
		if (k < 0.0f) // reflect to the current medium
		{
			return computeReflection(scene, mat, ray.direction, i.hitPoint, i.hitNormal, depth + 1, maxDepth,
				mediumIORs, terminatingColor);
		}
		else
		{
			float fresnel = computeFresnelTerm(ray.direction, i.hitNormal, eta);
			
			if (randomFloat() < fresnel) // reflect to the current medium
			{
				return computeReflection(scene, mat, ray.direction, i.hitPoint, i.hitNormal, depth + 1, maxDepth,
					mediumIORs, terminatingColor);
			}
			else // perform refraction
			{
				if (enteringMaterial)
					mediumIORs.push_back(targetIOR);
				else if (hasNonDefaultIOR)
					mediumIORs.pop_back();
				
				vec3 ideal;
				vec3 refractedRay = randomRefractedVector(ray.direction, i.hitNormal, eta, k, mat.roughness, ideal);
				
				auto deepBounce = gatherBouncesRecursive(scene, ray3d(i.hitPoint, refractedRay), depth + 1, maxDepth,
					mediumIORs, terminatingColor);
				
				mul(deepBounce, dot(refractedRay, ideal));
				return add(mat.emissiveColor, mul(mat.diffuseColor, deepBounce));
			}
		}
	}
	else if (randomFloat() > mat.roughness)
	{
		return computeReflection(scene, mat, ray.direction, i.hitPoint, i.hitNormal, depth + 1, maxDepth,
			mediumIORs, terminatingColor);
	}
	else
	{
		vec3 direction = randomDiffuseVector(i.hitNormal);
		
		auto deepBounce = gatherBouncesRecursive(scene, ray3d(i.hitPoint, direction), depth + 1, maxDepth,
			mediumIORs, terminatingColor);
		
		mul(deepBounce, dot(direction, i.hitNormal));
		
		return add(mat.emissiveColor, mul(mat.diffuseColor, deepBounce));
	}
	
	return vec4(1000.0f, 0.0f, 1000.0f, 0.0f);
}

void rt::raytrace(const RaytraceScene& scene, const et::vec2i& imageSize, const et::vec2i& origin,
	const et::vec2i& size, OutputFunction outputFunction)
{
	vec2i pixel = origin;
	vec2 dudv = vec2(2.0f) / vector2ToFloat(imageSize);
	vec2 subPixel = 0.5f * dudv;
	
	ray3d centerRay = scene.camera.castRay(vec2(0.0f));
	vec3 ce1 = cross(centerRay.direction, centerRay.direction.x > 0.1f ? unitY : unitX).normalized();
	vec3 ce2 = cross(ce1, centerRay.direction).normalized();
	
	auto it = scene.findNearestIntersection(centerRay);
	
	float deltaAngleForAppertureBlades = DOUBLE_PI / static_cast<float>(scene.apertureBlades);
	float initialAngleForAppertureBlades = 0.5f * deltaAngleForAppertureBlades;
	float focalDistance = length(it.hitPoint - centerRay.origin);
	plane focalPlane(scene.camera.direction(), (scene.camera.position() - scene.camera.direction() * focalDistance).length());
	
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		pixel.x = origin.x;
		outputFunction(pixel, vec4(1.0f, 0.0f, 0.0f, 1.0f));
		pixel.x = origin.x + size.x - 1;
		outputFunction(pixel, vec4(0.0f, 1.0f, 0.0f, 1.0f));
	}

	for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
	{
		pixel.y = origin.y;
		outputFunction(pixel, vec4(0.0f, 0.0f, 1.0f, 1.0f));
		pixel.y = origin.y + size.y - 1;
		outputFunction(pixel, vec4(1.0f, 0.0f, 1.0f, 1.0f));
	}
	
	IndexOfRefractionDeque iorDeque;
	
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
		{
			vec4 result;
			
			for (size_t sample = 0; sample < scene.options.samples; ++sample)
			{
				vec2 fpixel = (vector2ToFloat(pixel) + vec2(0.5f)) * dudv - vec2(1.0f);
				ray3d r = scene.camera.castRay(fpixel + subPixel * vec2(randomFloatFrom1to1(), randomFloatFrom1to1()));

				if (scene.apertureSize > 0.0f)
				{
					vec3 focal;
					intersect::rayPlane(r, focalPlane, &focal);

					float ra1 = initialAngleForAppertureBlades + static_cast<float>(rand() % scene.apertureBlades) * deltaAngleForAppertureBlades;
					float ra2 = ra1 + deltaAngleForAppertureBlades;
					float rd = scene.apertureSize * std::sqrt(randomFloat());
	
#				if (ET_PLATFORM_WIN)
					float cra1 = std::cos(ra1);
					float sra1 = std::sin(ra1);
					float cra2 = std::cos(ra2);
					float sra2 = std::sin(ra2);
#				else
					float cra1 = 0.0f;
					float sra1 = 0.0f;
					float cra2 = 0.0f;
					float sra2 = 0.0f;
					__sincosf(ra1, &sra1, &cra1);
					__sincosf(ra2, &sra2, &cra2);
#				endif
					
					vec3 o1 = rd * (ce1 * sra1 + ce2 * cra1);
					vec3 o2 = rd * (ce1 * sra2 + ce2 * cra2);
					vec3 cameraJitter = r.origin + mix(o1, o2, randomFloat());
					
					r = ray3d(cameraJitter, (focal - cameraJitter).normalized());
				}
			
				iorDeque.clear();
				iorDeque.push_back(1.0f);
				result += gatherBouncesRecursive(scene, r, 0, scene.options.bounces, iorDeque, vec4(0.0f));
			}
			
			result *= -scene.options.exposure / static_cast<float>(scene.options.samples);
			result = vec4(1.0f) - vec4(std::exp(result.x), std::exp(result.y), std::exp(result.z), 0.0f);
			outputFunction(pixel, result);
		}
	}
}

void rt::raytracePreview(const RaytraceScene& scene, const et::vec2i& imageSize, const et::vec2i& origin,
	const et::vec2i& size, OutputFunction outputFunction)
{
	const size_t previewSamples = 4;
	const size_t previewBounces = 2;
	
	vec2i pixel = origin;
	vec2 dudv = vec2(2.0f) / vector2ToFloat(imageSize);
	
	IndexOfRefractionDeque iorDeque;
	
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
		{
			vec4 result;
			vec2 fpixel = (vector2ToFloat(pixel) + vec2(0.5f)) * dudv - vec2(1.0f);
			
			for (size_t sample = 0; sample < previewSamples; ++sample)
			{
				iorDeque.clear();
				
				iorDeque.push_back(1.0f);
				
				result += gatherBouncesRecursive(scene, scene.camera.castRay(fpixel), 0, previewBounces,
					iorDeque, vec4(1.0f));
			}
			
			result /= static_cast<float>(previewSamples);
			
			outputFunction(pixel, result);
		}
	}
}

/*
 *
 * Service
 *
 */

vec3 randomDiffuseVector(const vec3& normal)
{
	return randomVectorOnHemisphere(normal, HALF_PI);
}

vec3 randomReflectedVector(const vec3& incidence, const vec3& normal, float roughness, vec3& idealReflection)
{
	idealReflection = reflect(incidence, normal);
	return randomVectorOnHemisphere(idealReflection, HALF_PI * roughness);
}

vec3 refract(const vec3& incidence, const vec3& normal, float eta, float k)
{
	return eta * incidence - (eta * dot(normal, incidence) + std::sqrt(k)) * normal;
}

vec3 randomRefractedVector(const vec3& incidence, const vec3& normal, float eta, float k, float roughness, vec3& idealRefraction)
{
	ET_ASSERT(k > 0.0f)
	idealRefraction = refract(incidence, normal, eta, k);
	return randomVectorOnHemisphere(idealRefraction, HALF_PI * roughness);
}

float calculateRefractiveCoefficient(const vec3& incidence, const vec3& normal, float eta)
{
	return 1.0f - sqr(eta) * (1.0f - sqr(dot(normal, incidence)));
}

float computeFresnelTerm(const vec3& incidence, const vec3& normal, float indexOfRefraction)
{
	float eta = indexOfRefraction * dot(incidence, normal);
	float eta2 = eta * eta;
	float beta = 1.0f - indexOfRefraction * indexOfRefraction;
	float result = 1.0f + 2.0f * (eta2 + eta * sqrt(beta + eta2)) / beta;
	return clamp(result * result, 0.0f, 1.0f);
}
