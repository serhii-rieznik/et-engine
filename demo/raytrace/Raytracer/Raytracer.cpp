//
//  Raytracer.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/collision/collision.h>
#include "Raytracer.h"

using namespace rt;
using namespace et;

vec3 randomVectorOnHemisphere(const vec3& base, float distribution);
vec4 performRaytracing(const RaytraceScene& scene, const ray3d& ray);

vec4 computeColorSequence(const RaytraceScene& scene, ray3d ray, const SceneObject& object,
	int lightBounce, std::vector<SceneObject>& hits);

int findNearestIntersection(const RaytraceScene& scene, const ray3d& ray, vec4& color, vec3& normal, vec3& point);
int findNearestIntersection(const RaytraceScene& scene, const ray3d& ray, vec3& normal, vec3& point);

int findNearestIntersection(const RaytraceScene& scene, const ray3d& ray, vec3& normal, vec3& point)
{
	normal = vec3(0.0f);
	
	int index = 0;
	int result = RaytraceScene::missingObject;
	
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
				normal = obj.normalFromPoint(pt);
			}
		}
		
		++index;
	}
	
	if (result != RaytraceScene::missingObject)
		normal.normalize();
	
	return result;
}


int findNearestIntersection(const RaytraceScene& scene, const ray3d& ray, vec4& color, vec3& normal, vec3& point)
{
	color = vec4(0.0f);
	normal = vec3(0.0f);
	
	int index = 0;
	int result = RaytraceScene::missingObject;
	
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
	
	if (result != RaytraceScene::missingObject)
		normal.normalize();
	
	return result;
}

void gatherBounces(const RaytraceScene& scene, ray3d ray, std::vector<SceneObject>& hits, int lightBounce)
{
	if (lightBounce >= scene.options.bounces) return;
	
	vec3 hitPoint;
	vec3 hitNormal;
	
	int bounces = 0;
	auto obj = scene.objectAtIndex(findNearestIntersection(scene, ray, hitNormal, hitPoint));
	while ((bounces < scene.options.bounces) && (obj.objectClass != SceneObject::Class_None) && (obj.objectClass != SceneObject::Class_SphericalLight))
	{
		hits.push_back(obj);
		ray.direction = randomVectorOnHemisphere(reflect(hitPoint - ray.origin, hitNormal).normalized(), obj.roughness);
		ray.origin = hitPoint;
		obj = scene.objectAtIndex(findNearestIntersection(scene, ray, hitNormal, hitPoint));
		++bounces;
	}
	hits.push_back(obj);
}

vec4 performRaytracing(const RaytraceScene& scene, const ray3d& ray)
{
	std::vector<SceneObject> hits;
	hits.reserve(scene.options.bounces);
	
	vec4 result;
	
	for (int i = 0; i < scene.options.samples; ++i)
	{
		hits.clear();
		gatherBounces(scene, ray, hits, 0);
		
		if (hits.size() > 0)
		{
			vec4 intermediate;
			
			for (auto i = hits.rbegin(), e = hits.rend(); i != e; ++i)
				intermediate = intermediate * i->color + i->emissive;
			
			result += intermediate;
		}
	}
	
	result *= -scene.options.exposure / static_cast<float>(scene.options.samples);
	return vec4(1.0f) - vec4(std::exp(result.x), std::exp(result.y), std::exp(result.z), 0.0f);
}

void rt::raytrace(const RaytraceScene& scene, const et::vec2i& imageSize, const et::vec2i& origin,
	const et::vec2i& size, bool aa, OutputFunction output)
{
	vec2i pixel = origin;
	vec2 dudv = vec2(2.0f) / vector2ToFloat(imageSize);
	vec2 subPixel = 0.25f * dudv;
	
	for (pixel.y = origin.y; pixel.y < origin.y + size.y; ++pixel.y)
	{
		for (pixel.x = origin.x; pixel.x < origin.x + size.x; ++pixel.x)
		{
			vec4 result;
			vec2 fpixel = vector2ToFloat(pixel) * dudv - vec2(1.0f);
			
			if (aa)
			{
				fpixel += 0.5f * subPixel;
				
				result = 0.25f * (performRaytracing(scene, scene.camera.castRay(fpixel + subPixel * vec2(-1.0f, -1.0f))) +
					performRaytracing(scene, scene.camera.castRay(fpixel + subPixel * vec2(-1.0f,  1.0f))) +
					performRaytracing(scene, scene.camera.castRay(fpixel + subPixel * vec2( 1.0f, -1.0f))) +
					performRaytracing(scene, scene.camera.castRay(fpixel + subPixel * vec2( 1.0f,  1.0f))));
			}
			else
			{
				result = performRaytracing(scene, scene.camera.castRay(fpixel + subPixel));
			}
			
			output(pixel, result);
		}
	}
}

/*
 * Service
 */

vec3 randomVectorOnHemisphere(const vec3& w, float distribution)
{
	float r1 = randomFloat(-PI, PI);
	float r2 = randomFloat(0.0f, distribution);
	vec3 u = cross((std::abs(w.x) > 0.1f) ? unitY : unitX, w).normalized();
	return (u * std::cos(r1) + cross(w, u) * std::sin(r1)) * std::sqrt(r2) + w * std::sqrt(1.0f - r2);
}
