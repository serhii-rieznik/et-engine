//
//  Raytracer.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/collision/collision.h>
#include "Raytracer.h"

#include <Accelerate/Accelerate.h>

#include <pmmintrin.h>

using namespace rt;
using namespace et;

vec3 randomVectorOnHemisphere(const vec3& base, float distribution);
vec3 randomDiffuseVector(const vec3& base);
vec3 randomSpecularVector(const vec3& incidence, const vec3& base, float roughness);

float phong(const vec3& incidence, const vec3& reflected, const vec3& normal, float exponent);

vec4 performRaytracing(const RaytraceScene& scene, const ray3d& ray);

vec4 computeColorSequence(const RaytraceScene& scene, ray3d ray, const SceneObject& object,
	int lightBounce, std::vector<SceneObject>& hits);

Intersection findNearestIntersection(const RaytraceScene& scene, const ray3d& ray)
{
	Intersection result;
	
	int index = 0;
	vec3 hitPoint;
	
	float distance = std::numeric_limits<float>::max();
	for (const auto& obj : scene.objects)
	{
		if (obj.intersectsRay(ray, hitPoint))
		{
			float d = (hitPoint - ray.origin).dotSelf();
			if (d < distance)
			{
				distance = d;
				result = index;
				result.hitPoint = hitPoint;
			}
		}
		++index;
	}
	
	result.hitNormal = scene.objectAtIndex(result.objectIndex).normalFromPoint(result.hitPoint);
	return result;
}

int findNearestIntersection(const RaytraceScene& scene, const ray3d& ray, vec4& color, vec3& normal, vec3& point)
{
	color = vec4(0.0f);
	normal = vec3(0.0f);
	
	int index = 0;
	int result = Intersection::missingObject;
	
	float distance = std::numeric_limits<float>::max();
	for (const auto& obj : scene.objects)
	{
		vec3 pt;
		if (obj.intersectsRay(ray, pt))
		{
			float d = (pt - ray.origin).dotSelf();
			if (d < distance)
			{
				result = index;
				distance = d;
				point = pt;
				color = obj.color;
				normal = obj.normalFromPoint(pt);
			}
		}
		
		++index;
	}
	
	if (result != Intersection::missingObject)
		normal.normalize();
	
	return result;
}

void gatherBounces(const RaytraceScene& scene, ray3d ray, Intersection::List& hits)
{
	auto intersection = findNearestIntersection(scene, ray);
	
	while (hits.size() < scene.options.bounces)
	{
		intersection.outgoingRay = ray3d(intersection.hitPoint, randomDiffuseVector(intersection.hitNormal));
		hits.push_back(intersection);
		intersection = findNearestIntersection(scene, intersection.outgoingRay);
	}
	
	hits.push_back(intersection);
}

vec4 gatherBouncesRecursive(const RaytraceScene& scene, const ray3d& ray, int depth)
{
	if (depth >= scene.options.bounces)
		return vec4(0.0f);
	
	auto i = findNearestIntersection(scene, ray);
	const auto& obj = scene.objectAtIndex(i.objectIndex);
	
	if (obj.objectClass == SceneObject::Class_None)
		return vec4(1.0);
	
	vec3 newDirection;
	float scale = 0.0f;
	if (randomFloat(0.0f, 1.0f) > obj.roughness)
	{
		newDirection = randomSpecularVector(ray.direction, i.hitNormal, obj.roughness);
		scale = dot(newDirection, reflect(ray.direction, i.hitNormal));
	}
	else
	{
		newDirection = randomDiffuseVector(i.hitNormal);
		scale = dot(newDirection, i.hitNormal);
	}
	
	return obj.emissive +
		scale * (obj.color * gatherBouncesRecursive(scene, ray3d(i.hitPoint, newDirection), depth + 1));
}

void rt::raytrace(const RaytraceScene& scene, const et::vec2i& imageSize, const et::vec2i& origin,
	const et::vec2i& size, bool aa, OutputFunction output)
{
	vec2i pixel = origin;
	vec2 dudv = vec2(2.0f) / vector2ToFloat(imageSize);
	vec2 subPixel = 0.5f * dudv;
	
	ray3d centerRay = scene.camera.castRay(vec2(0.0f));
	vec3 ce1 = cross(centerRay.direction, centerRay.direction.x > 0.1f ? unitY : unitX).normalized();
	vec3 ce2 = cross(ce1, centerRay.direction).normalized();
	
	float focalDistance = 0.775f * scene.camera.position().length();
	float aperture = 0.0f;
	
	plane focalPlane(scene.camera.direction(), (scene.camera.position() - scene.camera.direction() * focalDistance).length());
 
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		pixel.x = origin.x;
		output(pixel, vec4(1.0f, 0.0f, 0.0f, 1.0f));
		pixel.x = origin.x + size.x - 1;
		output(pixel, vec4(1.0f, 0.0f, 0.0f, 1.0f));
	}

	for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
	{
		pixel.y = origin.y;
		output(pixel, vec4(1.0f, 0.0f, 0.0f, 1.0f));
		pixel.y = origin.y + size.y - 1;
		output(pixel, vec4(1.0f, 0.0f, 0.0f, 1.0f));
	}
	
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
		{
			vec4 result;
			
			for (int sample = 0; sample < scene.options.samples; ++sample)
			{
				vec2 fpixel = (vector2ToFloat(pixel) + vec2(0.5f)) * dudv - vec2(1.0f);
				ray3d r = scene.camera.castRay(fpixel + subPixel * vec2(randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f)));
				
				if (aperture > 0.0f)
				{
					vec3 focal;
					intersect::rayPlane(r, focalPlane, &focal);
					float rd = randomFloat(0.0f, aperture);
					float ra = randomFloat(-PI, PI);
					vec3 cameraJitter = r.origin + rd * (ce1 * std::sin(ra) + ce2 * std::cos(ra));
					ray3d finalRay(cameraJitter, normalize(focal - cameraJitter));
					result += gatherBouncesRecursive(scene, finalRay, 0);
				}
				else
				{
					result += gatherBouncesRecursive(scene, r, 0);
				}
			}
			
			result *= -scene.options.exposure / static_cast<float>(scene.options.samples);
			result = vec4(1.0f) - vec4(std::exp(result.x), std::exp(result.y), std::exp(result.z), 0.0f);
			
			output(pixel, result);
		}
	}
}

/*
 * Service
 */

vec3 randomVectorOnHemisphere(const vec3& w, float distribution)
{
	float cr1 = 0.0f;
	float sr1 = 0.0f;
	float r2 = randomFloat(0.0f, distribution);

	__sincosf(randomFloat(-PI, PI), &sr1, &cr1);
	vec3 u = cross((std::abs(w.x) > 0.1f) ? unitY : unitX, w).normalized();
	return (u * cr1 + cross(w, u) * sr1) * std::sqrt(r2) + w * std::sqrt(1.0f - r2);
}

vec3 randomDiffuseVector(const vec3& base)
{
	return randomVectorOnHemisphere(base, 1.0f);
}

vec3 randomSpecularVector(const vec3& incidence, const vec3& base, float roughness)
{
	return randomVectorOnHemisphere(reflect(incidence, base), std::sin(HALF_PI * roughness));
}

float phong(const vec3& incidence, const vec3& reflected, const vec3& normal, float exponent)
{
	float s = dot(reflect(incidence, normal), reflected);
	return (s <= 0.0f) ? 0.0f : std::pow(s, exponent);
}