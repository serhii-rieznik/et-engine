/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/sensor/location.h>

#if (ET_PLATFORM_IOS)

#include <CoreLocation/CoreLocation.h>
#include <et/platform-apple/apple.h>

@interface LocationManagerProxy : NSObject<CLLocationManagerDelegate>
{
    et::LocationManagerPrivate* _p;
    CLLocationManager* _man;
}

- (id)initWithPrivate:(et::LocationManagerPrivate*)p;

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation;
- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error;

@property (nonatomic, readonly) CLLocationManager* man;

@end

namespace et
{
	class LocationManagerPrivate : public EventReceiver
	{
	public:
		LocationManagerPrivate(bool e) : enabled(false)
		{ 
            _proxy = [[LocationManagerProxy alloc] initWithPrivate:this];
			setEnabled(e);
		};
        
        ~LocationManagerPrivate()
        {
			ET_OBJC_RELEASE(_proxy);
        }
        
		void setEnabled(bool e)
		{
			if (enabled == e) return;
			
			enabled = e;
			
			if (enabled)
				[_proxy.man startUpdatingLocation];
			else
				[_proxy.man stopUpdatingLocation];
		}

	public:
        LocationManagerProxy* _proxy;
		LocationManager* manager;
		bool enabled;
	};
}

using namespace et;

/*
 * Proxy implementation
 */

@implementation LocationManagerProxy

@synthesize man = _man;

- (id)initWithPrivate:(et::LocationManagerPrivate*)p
{
    self = [super init];
    if (self)
    {
        _p = p;
        _man = [[CLLocationManager alloc] init];
        _man.delegate = self;
    }
    return self;
}

- (void)dealloc
{
	ET_OBJC_RELEASE(_man);
	
#if (!ET_OBJC_ARC_ENABLED)
    [super dealloc];
#endif
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
    et::Location l;
    
    l.latitude = newLocation.coordinate.latitude;
    l.longitude = newLocation.coordinate.longitude;
    l.altitude = newLocation.altitude;
    l.timestamp = [newLocation.timestamp timeIntervalSince1970];
    
    _p->manager->locationUpdated.invoke(l);
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
}

@end

LocationManager::LocationManager() : _private(new LocationManagerPrivate(false))
{
	_private->manager = this;
}

LocationManager::~LocationManager()
{
	delete _private;
}

void LocationManager::setEnabled(bool e)
{
	_private->setEnabled(e);
}

bool LocationManager::enabled() const
{
	return _private->enabled;
}

#endif // ET_PLATFORM_IOS
