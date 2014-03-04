/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/scene3d/animation.h>

using namespace et;
using namespace et::s3d;

Animation::Animation()
{
}

Animation::Animation(const std::string& n) :
	_name(n)
{
}

void Animation::addKeyFrame(float t, const ComponentTransformable& c)
{
	_times.push_back(t);
	_transformations.push_back(c);
}

void Animation::setTimeRange(float start, float stop)
{
	_startTime = start;
	_stopTime = stop;
	
	// TODO : validate values stored in _times and _transformations
}

void Animation::setFrameRate(float r)
{
	_frameRate = r;
}

void Animation::transformation(float time, vec3& t, quaternion& o, vec3& s)
{
	float d = duration();
	
	while (time < _startTime)
		time += d;
	
	while (time > _stopTime)
		time -= d;
	
	auto lTime = std::lower_bound(_times.begin(), _times.end(), time);
	auto uTime = std::upper_bound(_times.begin(), _times.end(), time);
	
	auto lTransform = _transformations.at(lTime - _times.begin());
	auto uTransform = _transformations.at(uTime - _times.begin());
	
	float interolationFactor = (time - *lTime) / (*uTime - *lTime);
	
	t = mix(lTransform.translation(), uTransform.translation(), interolationFactor);
	o = slerp(lTransform.orientation(), uTransform.orientation(), interolationFactor);
	s = mix(lTransform.translation(), uTransform.translation(), interolationFactor);
}

mat4 Animation::transformation(float time)
{
	vec3 t;
	quaternion o;
	vec3 s;
	
	transformation(time, t, o, s);
	
	ComponentTransformable result;
	result.setTranslation(t);
	result.setOrientation(o);
	result.setScale(s);
	return result.transform();
}
