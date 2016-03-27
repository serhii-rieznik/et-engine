/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/cameraorbitcontroller.h>

using namespace et;

CameraOrbitController::CameraOrbitController(Camera& cam, bool connectInput) :
	CameraController(cam, connectInput)
{
	camera().lockUpVector(unitY);
	
	_anglesDistanceAnimator.valueUpdated.connect([this](const vec3& p)
		{ _shouldRebuildMatrix = true; });
	_anglesDistanceAnimator.valueUpdated.connect([this](const vec3& p)
		{ _shouldRebuildMatrix = true; });

	_gestures.drag.connect([this](const GesturesRecognizer::DragGesture& drag)
	{
		if (drag.pointerType == PointerType_General)
		{
			vec3 currentValue = _anglesDistanceAnimator.targetValue();
			vec2 delta(currentValue.x + drag.delta.x, currentValue.y - drag.delta.y);
			validateCameraAngles(delta);
			_anglesDistanceAnimator.setTargetValue(vec3(delta, currentValue.z));
		}
	});
	
	_gestures.zoom.connect([this](float z)
	{
		vec3 newValue = _anglesDistanceAnimator.targetValue();
		newValue.z *= z;
		_anglesDistanceAnimator.setTargetValue(newValue);
	});
	
	_gestures.scroll.connect([this](vec2 value, PointerOrigin origin)
	{
		if (origin == PointerOrigin_Mouse)
		{
			vec3 newValue = _anglesDistanceAnimator.targetValue() + vec3(0.0f, 0.0f, value.y);
			_anglesDistanceAnimator.setTargetValue(newValue);
		}
	});
}

void CameraOrbitController::startUpdates()
{
	CameraController::startUpdates();
	_anglesDistanceAnimator.run();
	_targetPoint.run();
}

void CameraOrbitController::cancelUpdates()
{
	CameraController::cancelUpdates();
	_anglesDistanceAnimator.cancelUpdates();
	_targetPoint.cancelInterpolation();
}

void CameraOrbitController::onKeyPressed(size_t key)
{
	_pressedKeys[key] = 1;
}

void CameraOrbitController::onKeyReleased(size_t key)
{
	_pressedKeys[key] = 0;
}

void CameraOrbitController::onPointerPressed(PointerInputInfo info)
{
	_gestures.onPointerPressed(info);
}

void CameraOrbitController::onPointerMoved(PointerInputInfo info)
{
	_gestures.onPointerMoved(info);
}

void CameraOrbitController::onPointerReleased(PointerInputInfo info)
{
	_gestures.onPointerReleased(info);
}

void CameraOrbitController::onPointerCancelled(PointerInputInfo info)
{
	_gestures.onPointerCancelled(info);
}

void CameraOrbitController::onPointerScrolled(PointerInputInfo info)
{
	_gestures.onPointerScrolled(info);
}

void CameraOrbitController::update(float)
{
	if (_shouldRebuildMatrix)
	{
		const auto& a = _anglesDistanceAnimator.value();
		const auto& t = _targetPoint.value();
		camera().lookAt(t + fromSpherical(a.y, a.x) * a.z, t);
		_shouldRebuildMatrix = false;
	}

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
		vec3 currentValue = _anglesDistanceAnimator.targetValue();
		vec2 delta(currentValue.x - side * DEG_1, currentValue.y + directional * DEG_1);
		validateCameraAngles(delta);
		_anglesDistanceAnimator.setTargetValue(vec3(delta, currentValue.z));
	}
}

void CameraOrbitController::setMovementSpeed(const vec3& speed)
{
	_movementSpeed = speed;
}

void CameraOrbitController::setIntepolationRate(float rate)
{
	_anglesDistanceAnimator.setRate(rate);
	_targetPoint.setRate(rate);
}

void CameraOrbitController::validateCameraAngles(vec2& angles)
{
	angles.y = clamp(angles.y, -HALF_PI + DEG_15, HALF_PI - DEG_15);
}

void CameraOrbitController::setTargetPoint(const vec3& tp)
{
	_targetPoint.setTargetValue(tp);
}

void CameraOrbitController::synchronize(const Camera& cam)
{
	_anglesDistanceAnimator.setTargetValue(toSpherical(cam.position() - _targetPoint.targetValue()));
	_anglesDistanceAnimator.finishInterpolation();
}
