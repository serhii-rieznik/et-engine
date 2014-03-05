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
	ET_ASSERT(stop - start >= 0.0f);
	
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
	if (d == 0.0f)
	{
		auto mainTransform = _transformations.front();
		t = mainTransform.translation();
		o = mainTransform.orientation();
		s = mainTransform.scale();
		return;
	}
	
	while (time < _startTime)
		time += d;
	
	while (time >= _stopTime)
		time -= d;
	
	int nearestLowerFrame = static_cast<int>(_times.size()) - 1;
	for (auto i = _times.rbegin(), e = _times.rend(); (i != e) && (nearestLowerFrame >= 0); ++i)
	{
		if (*i <= time) break;
		--nearestLowerFrame;
	}
	int nearestUpperFrame = (nearestLowerFrame + 1 > _transformations.size()) ?
		nearestLowerFrame : nearestLowerFrame + 1;
	
	auto lTransform = _transformations.at(nearestLowerFrame);
	auto uTransform = _transformations.at(nearestUpperFrame);
	
	float interolationFactor = (time - _times.at(nearestLowerFrame)) /
		(_times.at(nearestUpperFrame) - _times.at(nearestLowerFrame));
	
	t = mix(lTransform.translation(), uTransform.translation(), interolationFactor);
	o = slerp(lTransform.orientation(), uTransform.orientation(), interolationFactor);
	s = mix(lTransform.scale(), uTransform.scale(), interolationFactor);
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
