/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/interpolationvalue.h>
#include <et/camera/cameracontroller.h>
#include <et/input/gestures.h>

namespace et
{
class CameraOrbitController : public CameraController
{
public:
	ET_DECLARE_POINTER(CameraOrbitController);

public:
	CameraOrbitController(const Camera::Pointer&, bool autoConnectToEvents);

	void setTargetPoint(const vec3&);
	void setMovementSpeed(const vec3&);
	void setIntepolationRate(float);

	void startUpdates() override;
	void cancelUpdates() override;

	void synchronize(const Camera::Pointer&) override;

private:
	void update(float) override;

	void onKeyPressed(uint32_t) override;
	void onKeyReleased(uint32_t) override;
	void onPointerPressed(PointerInputInfo) override;
	void onPointerMoved(PointerInputInfo) override;
	void onPointerReleased(PointerInputInfo) override;
	void onPointerCancelled(PointerInputInfo) override;
	void onPointerScrolled(PointerInputInfo) override;

	void validateCameraAngles(vec2&);

private:
	GesturesRecognizer _gestures;
	std::map<uint32_t, int> _pressedKeys;
	InterpolationValue<vec3> _anglesDistanceAnimator;
	InterpolationValue<vec3> _targetPoint;
	vec3 _movementSpeed = vec3(1.0f);
	bool _shouldRebuildMatrix = false;
};
}
