/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>

namespace et
{
	struct Location
	{
		float latitude;
		float longitude;
		float altitude;
		float timestamp;

		Location() : 
			latitude(0.0f), longitude(0.0f), altitude(0.0f), timestamp(0.0f) { }
		Location(float lat, float lon, float t) : 
			latitude(lat), longitude(lon), altitude(0.0f), timestamp(t) { }
		Location(float lat, float lon, float alt, float t) : 
			latitude(lat), longitude(lon), altitude(alt), timestamp(t) { }
	};

	class LocationManagerPrivate;
	class LocationManager
	{
	public:
		LocationManager();
		~LocationManager();

		void setEnabled(bool e);
		bool enabled() const;

		ET_DECLARE_EVENT1(locationUpdated, Location);

	private:
		LocationManagerPrivate* _private;
	};

}