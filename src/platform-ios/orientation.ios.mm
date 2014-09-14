/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/sensor/orientation.h>

#if (ET_PLATFORM_IOS)

#include <CoreMotion/CoreMotion.h>
#include <et/platform-apple/apple.h>

namespace et
{
	class OrientationManagerPrivate : public EventReceiver
	{
	private:
		static mat4 transformFromCMRotationMatrix(const CMRotationMatrix& m)
		{
			mat4 mout = identityMatrix;
			mout[0][0] = static_cast<float>(m.m11);
			mout[0][1] = static_cast<float>(m.m21);
			mout[0][2] = static_cast<float>(m.m31);
			mout[1][0] = static_cast<float>(m.m12);
			mout[1][1] = static_cast<float>(m.m22);
			mout[1][2] = static_cast<float>(m.m32);
			mout[2][0] = static_cast<float>(m.m13);
			mout[2][1] = static_cast<float>(m.m23);
			mout[2][2] = static_cast<float>(m.m33);
			return mout;
		}
		
	public:
		OrientationManagerPrivate() :
			manager(0), accelEnabled(false), gyroEnabled(false), updating(false)
		{
			_oq = [[NSOperationQueue alloc] init];
			_motionManager = [[CMMotionManager alloc] init];
			_motionManager.deviceMotionUpdateInterval = 1.0f / 100.0f;
		}
		
		~OrientationManagerPrivate()
		{
			ET_OBJC_RELEASE(_motionManager);
			ET_OBJC_RELEASE(_oq);
		}
		
		void update()
		{
			bool shouldSwitch = updating != (gyroEnabled || accelEnabled);
			if (!shouldSwitch) return;
			
			updating = gyroEnabled || accelEnabled;
			if (updating)
			{
				__block float t = 0.0f;
				
				[_motionManager startDeviceMotionUpdatesToQueue:_oq withHandler:^(CMDeviceMotion *motion, NSError *error)
				{
					float dt = (t == 0.0f) ? 0.0f : (motion.timestamp - t);

					if (gyroEnabled)
					{
						et::GyroscopeData d;

						d.interval = dt;
						d.timestamp = motion.timestamp;
						d.rate = vec3(motion.rotationRate.x, motion.rotationRate.y, motion.rotationRate.z);
						d.orientation = vec3(motion.attitude.pitch, motion.attitude.yaw, motion.attitude.roll);
						d.orientationMatrix = transformFromCMRotationMatrix(motion.attitude.rotationMatrix);
						d.orientationQuaternion = quaternion(motion.attitude.quaternion.w, motion.attitude.quaternion.x,
							motion.attitude.quaternion.y, motion.attitude.quaternion.z);
						manager->gyroscopeDataUpdated.invokeInMainRunLoop(d);
					}

					if (accelEnabled)
					{
						et::AccelerometerData d;
						d.value.x = motion.gravity.x;
						d.value.y = motion.gravity.y;
						d.value.z = motion.gravity.z;
						d.timestamp = motion.timestamp;
						d.interval = dt;
						manager->accelerometerDataUpdated.invokeInMainRunLoop(d);
					}

					t = motion.timestamp;
				}];
			}
			else
			{
				[_motionManager stopDeviceMotionUpdates];
			}
		}
		
		void setGyroEnabled(bool e)
		{
			if (gyroEnabled == e) return;
			gyroEnabled = e;
			update();
		}
		
		void setAccelEnabled(bool e)
		{
			if (accelEnabled == e) return;
			accelEnabled = e;
			update();
		}
		
	public:
		NSOperationQueue* _oq;
		CMMotionManager* _motionManager;
		OrientationManager* manager;
		
		bool accelEnabled;
		bool gyroEnabled;
		bool updating;
	};
}

using namespace et;

bool OrientationManager::accelerometerAvailable()
	{ return ET_OBJC_AUTORELEASE([[CMMotionManager alloc] init]).accelerometerAvailable; }

bool OrientationManager::gyroscopeAvailable()
	{ return ET_OBJC_AUTORELEASE([[CMMotionManager alloc] init]).gyroAvailable; }

OrientationManager::OrientationManager() : _private(new OrientationManagerPrivate)
	{ _private->manager = this; }

OrientationManager::~OrientationManager()
	{ delete _private; }

void OrientationManager::setAccelerometerEnabled(bool e)
	{ _private->setAccelEnabled(e); }

bool OrientationManager::accelerometerEnabled() const
	{ return _private->accelEnabled; }

void OrientationManager::setGyroscopeEnabled(bool e)
	{ _private->setGyroEnabled(e); }

bool OrientationManager::gyroscopeEnabled() const
	{ return _private->gyroEnabled; }

#endif // ET_PLATFORM_IOS
