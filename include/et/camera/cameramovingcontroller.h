/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/timers/interpolationvalue.h>
#include <et/camera/cameracontroller.h>

namespace et
{
	class CameraMovingController : public CameraController
	{
	public:
		CameraMovingController(Camera&, bool autoConnectToEvents);
		
		void startUpdates();
		void cancelUpdates();
		
	private:
		void update(float);
		void onKeyPressed(size_t);
		void onKeyReleased(size_t);
		void onPointerPressed(PointerInputInfo);
		void onPointerMoved(PointerInputInfo);
		void onPointerReleased(PointerInputInfo);
		void onPointerCancelled(PointerInputInfo);
		
	private:
		std::map<size_t, int> _pressedKeys;
		InterpolationValue<vec3> _positionAnimator;
	};
}
