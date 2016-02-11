/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/cameramovingcontroller.h>

using namespace et;

CameraMovingController::CameraMovingController(Camera& cam, bool connectInput) :
	CameraController(cam, connectInput)
{
	camera().lockUpVector(unitY);
	
	_positionAnimator.valueUpdated.connect([this](const vec3& p)
	{
		camera().setPosition(p);
	});
	
	_directionAnimator.valueUpdated.connect([this](const vec2& p)
	{
		camera().setDirection(fromSpherical(p.y, p.x));
	});
	
	_gestures.drag.connect([this](const GesturesRecognizer::DragGesture& drag)
	{
		if (pointerEventsEnabled() && (drag.pointerType == PointerType_General))
		{
			vec2 delta = _directionAnimator.targetValue() + vec2(drag.delta.x, -drag.delta.y);
			validateCameraAngles(delta);
			_directionAnimator.setTargetValue(delta);
		}
	});
}

void CameraMovingController::startUpdates()
{
	CameraController::startUpdates();
	synchronize(camera());
	_directionAnimator.run();
	_positionAnimator.run();
}

void CameraMovingController::cancelUpdates()
{
	CameraController::cancelUpdates();
	_positionAnimator.cancelUpdates();
	_directionAnimator.cancelUpdates();
}

void CameraMovingController::onKeyPressed(size_t key)
{
	_pressedKeys[key] = 1;
}

void CameraMovingController::onKeyReleased(size_t key)
{
	_pressedKeys[key] = 0;
}

void CameraMovingController::onPointerPressed(PointerInputInfo info)
{
	_gestures.onPointerPressed(info);
}

void CameraMovingController::onPointerMoved(PointerInputInfo info)
{
	_gestures.onPointerMoved(info);
}

void CameraMovingController::onPointerReleased(PointerInputInfo info)
{
	_gestures.onPointerReleased(info);
}

void CameraMovingController::onPointerCancelled(PointerInputInfo info)
{
	_gestures.onPointerCancelled(info);
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
		float multiplier = dt * static_cast<float>(1.0f + _pressedKeys.count(ET_KEY_SHIFT));
		vec3 direction = camera().side() * movement.x - camera().direction() * movement.z;
		_positionAnimator.addTargetValue(_movementSpeed * multiplier * direction);
	}
}

void CameraMovingController::setMovementSpeed(const vec3& speed)
{
	_movementSpeed = speed;
}

void CameraMovingController::setIntepolationRate(float rate)
{
	_positionAnimator.setRate(rate);
	_directionAnimator.setRate(rate);
}

void CameraMovingController::validateCameraAngles(vec2& angles)
{
	angles.y = clamp(angles.y, -HALF_PI + DEG_15, HALF_PI - DEG_15);
}

void CameraMovingController::synchronize(const Camera&)
{
	vec2 angles = toSpherical(camera().direction()).xy();
	_directionAnimator.setValue(angles);
	_directionAnimator.setTargetValue(angles);
	_positionAnimator.setValue(camera().position());
	_positionAnimator.setTargetValue(camera().position());
}
