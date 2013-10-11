#include <CoreLocation/CoreLocation.h>

#include <et/sensor/location.h>

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
            [_proxy release];
        }
        
		void setEnabled(bool e)
		{
			if (enabled == e) return;
			enabled = e;
            
            [_proxy.man performSelector:(enabled ? @selector(startUpdatingLocation) : @selector(stopUpdatingLocation))];
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
    [_man release];
    [super dealloc];
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
