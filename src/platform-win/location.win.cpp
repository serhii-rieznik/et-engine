#include <et/app/application.h>
#include <et/geometry/geometry.h>
#include <et/timers/notifytimer.h>
#include <et/sensor/location.h>

namespace et
{
	class LocationManagerPrivate : public EventReceiver
	{
	public:
		LocationManagerPrivate(LocationManager* m, bool e) :
			_man(m), _defaultLocation(49.980171f, 36.349479f, 0.0f, 0.0f), enabled(false)
		{ 
			_timer.expired.connect(this, &LocationManagerPrivate::timerExpired);
			setEnabled(e);
		};

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
		LocationManager* _man = nullptr;
		NotifyTimer _timer;
		Location _defaultLocation;
		bool enabled = false;
	};
}

using namespace et;

LocationManager::LocationManager()
{
	ET_PIMPL_INIT(LocationManager, this, false)
}

LocationManager::~LocationManager()
{
	ET_PIMPL_FINALIZE(LocationManager)
}

void LocationManager::setEnabled(bool e)
{
	_private->setEnabled(e);
}

bool LocationManager::enabled() const
{
	return _private->enabled;
}
