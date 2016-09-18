#include <et/app/application.h>
#include <et/core/notifytimer.h>
#include <et-ext/sensor/orientation.h>

namespace et
{
	class OrientationManagerPrivate : public EventReceiver
	{
	public:
		OrientationManagerPrivate(OrientationManager* m) :
			_man(m), accelEnabled(false), gyroEnabled(false)
		{
			_timerAccel.expired.connect(this, &OrientationManagerPrivate::onTimerExpired);
			_timerGyro.expired.connect(this, &OrientationManagerPrivate::onTimerExpired);
		}

		void setGyroEnabled(bool e)
		{
			if (gyroEnabled == e) return;	
			gyroEnabled = e;

			if (gyroEnabled)
				_timerGyro.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
			else
				_timerGyro.cancelUpdates();
		}

		void setAccelEnabled(bool e)
		{
			if (accelEnabled == e) return;	
			accelEnabled = e;

			if (accelEnabled)
				_timerAccel.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
			else
				_timerAccel.cancelUpdates();
		}

		void onTimerExpired(NotifyTimer* t)
		{
			if (t == &_timerAccel)
			{
				_man->accelerometerDataUpdated.invoke(_defaultAcceleration);
			}
			else if (t == &_timerGyro)
			{
				_man->gyroscopeDataUpdated.invoke(_defaultGyro);
			}
		}

	public:
		NotifyTimer _timerAccel;
		NotifyTimer _timerGyro;

		AccelerometerData _defaultAcceleration;
		GyroscopeData _defaultGyro;
		OrientationManager* _man = nullptr;
		
		bool accelEnabled = false;
		bool gyroEnabled = false;
	};
}

using namespace et;

OrientationManager::OrientationManager()
{
	ET_PIMPL_INIT(OrientationManager, this);
}

OrientationManager::~OrientationManager()
{
	ET_PIMPL_FINALIZE(OrientationManager)
}

void OrientationManager::setAccelerometerEnabled(bool e)
	{ _private->setAccelEnabled(e); }

bool OrientationManager::accelerometerEnabled() const
	{ return _private->accelEnabled; }

void OrientationManager::setGyroscopeEnabled(bool e)
	{ _private->setGyroEnabled(e); }

bool OrientationManager::gyroscopeEnabled() const
	{ return _private->gyroEnabled; }
