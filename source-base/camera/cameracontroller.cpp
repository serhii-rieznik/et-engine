/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/camera/cameracontroller.h>

using namespace et;

CameraController::CameraController(Camera& cam, bool connectInput) :
	_camera(cam)
{
	if (connectInput)
	{
		connectPointerEvents();
		connectKeyboardEvents();
	}
	
	_updateTimer.expired.connect([this](NotifyTimer* timer)
	{
		float currentTime = timer->actualTime();
		
		if (_lastUpdateTime == 0.0f)
			_lastUpdateTime = currentTime;
		
		update(currentTime - _lastUpdateTime);
		
		_lastUpdateTime = currentTime;
	});
}

void CameraController::startUpdates()
{
	_updateTimer.start(currentTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

void CameraController::cancelUpdates()
{
	_updateTimer.cancelUpdates();
}

void CameraController::setPointerEventsEnabled(bool enable)
{
	_pointerEventsEnabled = enable;
}

void CameraController::setKeyboardEventsEnabled(bool enable)
{
	_keyboardEventsEnabled = enable;
}

void CameraController::connectPointerEvents()
{
	input().pointerPressed.connect(this, &CameraController::onPointerPressed);
	input().pointerMoved.connect(this, &CameraController::onPointerMoved);
	input().pointerReleased.connect(this, &CameraController::onPointerCancelled);
	input().pointerCancelled.connect(this, &CameraController::onPointerCancelled);
}

void CameraController::disconnectPointerEvents()
{
	input().pointerPressed.disconnect(this);
	input().pointerMoved.disconnect(this);
	input().pointerReleased.disconnect(this);
	input().pointerCancelled.disconnect(this);
}

void CameraController::connectKeyboardEvents()
{
	input().keyPressed.connect(this, &CameraController::onKeyPressed);
	input().keyReleased.connect(this, &CameraController::onKeyReleased);
}

void CameraController::disconnectKeyboardEvents()
{
	input().keyPressed.disconnect(this);
	input().keyReleased.disconnect(this);
}
