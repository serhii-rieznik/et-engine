/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/serialization.h>
#include <et/scene3d/animation.h>

using namespace et;
using namespace et::s3d;

const int animationVersion_1 = 0x0001;
const int animationCurrentVersion = animationVersion_1;

Animation::Animation()
{
	
}

Animation::Animation(std::istream& stream)
{
	deserialize(stream);
}

void Animation::addKeyFrame(float t, const vec3& tr, const quaternion& o, const vec3& s)
{
	_frames.emplace_back(t, tr, o, s);
}

void Animation::setTimeRange(float start, float stop)
{
	ET_ASSERT(stop - start >= 0.0f);
	_startTime = start;
	_stopTime = stop;
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
		auto mainTransform = _frames.front();
		t = mainTransform.translation;
		o = mainTransform.orientation;
		s = mainTransform.scale;
		return;
	}
	
	while (time < _startTime)
		time += d;
	
	while (time >= _stopTime)
		time -= d;
	
	int nearestLowerFrame = static_cast<int>(_frames.size()) - 1;
	for (auto i = _frames.rbegin(), e = _frames.rend(); (i != e) && (nearestLowerFrame >= 0); ++i)
	{
		if (i->time <= time) break;
		--nearestLowerFrame;
	}
	int nearestUpperFrame = (nearestLowerFrame + 1 > _frames.size()) ?
		nearestLowerFrame : nearestLowerFrame + 1;
	
	const auto& lowerFrame = _frames.at(nearestLowerFrame);
	const auto& upperFrame = _frames.at(nearestUpperFrame);
		
	float interolationFactor =
		(time - lowerFrame.time) / (lowerFrame.time - upperFrame.time);
	
	t = mix(lowerFrame.translation, upperFrame.translation, interolationFactor);
	o = slerp(lowerFrame.orientation, upperFrame.orientation, interolationFactor);
	s = mix(lowerFrame.scale, upperFrame.scale, interolationFactor);
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

void Animation::serialize(std::ostream& stream) const
{
	serializeInt(stream, animationCurrentVersion);
	serializeInt(stream, 3 * sizeof(float) + 2 * sizeof(uint32_t) + _frames.size() * sizeof(Frame));
	serializeFloat(stream, _startTime);
	serializeFloat(stream, _stopTime);
	serializeFloat(stream, _frameRate);
	serializeInt(stream, _outOfTimeMode);
	serializeInt(stream, _frames.size());
	for (const auto& frame : _frames)
	{
		serializeFloat(stream, frame.time);
		serializeVector(stream, frame.translation);
		serializeQuaternion(stream, frame.orientation);
		serializeVector(stream, frame.scale);
	}
}

void Animation::deserialize(std::istream& stream)
{
	_frames.clear();
	
	int version = deserializeInt(stream);
	int dataSize = deserializeInt(stream);
	
	if (version == animationVersion_1)
	{
		_startTime = deserializeFloat(stream);
		_stopTime = deserializeFloat(stream);
		_frameRate = deserializeFloat(stream);
		_outOfTimeMode = static_cast<OutOfTimeMode>(deserializeInt(stream));
		
		int numFrames = deserializeInt(stream);
		
		_frames.reserve(numFrames);
		for (int i = 0; i < numFrames; ++i)
		{
			float t = deserializeFloat(stream);
			vec3 tr = deserializeVector<vec3>(stream);
			quaternion q = deserializeQuaternion(stream);
			vec3 s = deserializeVector<vec3>(stream);
			addKeyFrame(t, tr, q, s);
		}
	}
	else
	{
		stream.seekg(dataSize, std::ios_base::cur);
	}
}
