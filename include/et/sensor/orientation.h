/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>

namespace et
{
	struct AccelerometerData
	{
		vec3 value;
		float timestamp;
        float interval;

		AccelerometerData() : 
			timestamp(0.0f), interval(0.0f) { }

		AccelerometerData(const vec3& v) : 
			value(v), timestamp(0.0f), interval(0.0f) { }

		AccelerometerData(const vec3& v, float t) : 
			value(v), timestamp(t), interval(0.0f) { }
	};

	struct GyroscopeData
	{
		vec3 rate;
        vec3 orientation;
		quaternion orientationQuaternion;
		mat3 orientationMatrix;
        
		float timestamp = 0.0f;
		float interval = 0.0f;

		GyroscopeData() 
			{ }

		GyroscopeData(const vec3& r) : 
			rate(r) { }

		GyroscopeData(const vec3& r, float t) : 
			rate(r), timestamp(t) { }
	};

	class OrientationManagerPrivate;
	class OrientationManager
	{
	public:
		static bool accelerometerAvailable();
		static bool gyroscopeAvailable();

	public:
		OrientationManager();
		~OrientationManager();

		void setAccelerometerEnabled(bool e);
		bool accelerometerEnabled() const;

		void setGyroscopeEnabled(bool e);
		bool gyroscopeEnabled() const;

		ET_DECLARE_EVENT1(accelerometerDataUpdated, AccelerometerData)
		ET_DECLARE_EVENT1(gyroscopeDataUpdated, GyroscopeData)

	private:
		ET_DECLARE_PIMPL(OrientationManager, 384)
	};
}
