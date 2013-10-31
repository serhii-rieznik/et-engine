#include <et/app/application.h>
#include <et/geometry/geometry.h>
#include <et/timers/notifytimer.h>
#include <et/sensor/location.h>

namespace et
{
	class LocationManagerPrivate : public EventReceiver
	{
	public:
		LocationManagerPrivate(bool e) : _man(0), _defaultLocation(49.980171f, 36.349479f, 0.0f, 0.0f), enabled(false)
		{ 
			_timer.expired.connect(this, &LocationManagerPrivate::timerExpired);
			setEnabled(e);
		};

		void setOwner(LocationManager* m)
			{ _man = m; }

		void setEnabled(bool e)
		{
			if (enabled == e) return;
			enabled = e;

			if (enabled)
				_timer.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
			else
				_timer.cancelUpdates();
		}

		void timerExpired(NotifyTimer*)
		{
			_defaultLocation.latitude += randomFloat(-0.001f, 0.001f);
			_defaultLocation.longitude += randomFloat(-0.001f, 0.001f);
			_defaultLocation.timestamp = mainTimerPool()->actualTime();
			_man->locationUpdated.invoke(_defaultLocation);
		}

	public:
		LocationManager* _man;
		NotifyTimer _timer;
		Location _defaultLocation;
		bool enabled;
	};
}

using namespace et;

LocationManager::LocationManager() : _private(new LocationManagerPrivate(false))
{
	_private->setOwner(this);
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
