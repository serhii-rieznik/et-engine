/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/cameramovingcontroller.h>

using namespace et;

CameraMovingController::CameraMovingController(Camera& cam, bool connectInput) :
	CameraController(cam, connectInput)
{
	_positionAnimator.setValue(cam.position());
	_positionAnimator.valueUpdated.connect([this](const vec3& p)
	{
		camera().setPosition(p);
	});
}

void CameraMovingController::startUpdates()
{
	CameraController::startUpdates();
	_positionAnimator.setValue(camera().position());
	_positionAnimator.run();
}

void CameraMovingController::cancelUpdates()
{
	CameraController::cancelUpdates();
	_positionAnimator.cancelUpdates();
}

void CameraMovingController::onKeyPressed(size_t key)
{
	_pressedKeys[key] = 1;
}

void CameraMovingController::onKeyReleased(size_t key)
{
	_pressedKeys[key] = 0;
}

void CameraMovingController::onPointerPressed(PointerInputInfo)
{
	
}

void CameraMovingController::onPointerMoved(PointerInputInfo)
{
	
}

void CameraMovingController::onPointerReleased(PointerInputInfo)
{
	
}

void CameraMovingController::onPointerCancelled(PointerInputInfo)
{
	
}

void CameraMovingController::update(float dt)
{
	float directional = 0.0f;
	if (_pressedKeys.count(ET_KEY_W) && (_pressedKeys[ET_KEY_W] != 0))
		directional += 1.0f;
	if (_pressedKeys.count(ET_KEY_S) && (_pressedKeys[ET_KEY_S] != 0))
		directional -= 1.0f;

	float side = 0.0f;
	if (_pressedKeys.count(ET_KEY_D) && (_pressedKeys[ET_KEY_D] != 0))
		side += 1.0f;
	if (_pressedKeys.count(ET_KEY_A) && (_pressedKeys[ET_KEY_A] != 0))
		side -= 1.0f;
	
	vec3 movement(side, 0.0f, directional);
	if (movement.dotSelf() > 0.0f)
	{
		movement.normalize();
		_positionAnimator.addTargetValue(camera().side() * movement.x - camera().direction() * movement.z);
	}
}
