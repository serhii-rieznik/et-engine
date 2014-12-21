//
//  DemoCameraController.cpp
//  SceneRendering
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include "DemoCameraController.h"

using namespace et;
using namespace demo;

CameraController::CameraController()
{
	_mainCamera.lookAt(vec3(500.0f, 200.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f));
	_mainCamera.lockUpVector(unitY);
	
	_observerCamera.lookAt(vec3(3072.0f));
	
	_updateTimer.expired.connect([this](et::NotifyTimer*)
	{
		vec2 n = (_boostEnabled ? 5.0f : 1.0f) * _movements.normalized();
		vec3 pos = _mainCamera.position() + n.y * _mainCamera.side() - n.x * _mainCamera.direction();
		_mainCamera.setPosition(pos);
	});
}

void CameraController::init(et::RenderContext* rc)
{
	adjustCameraToNextContextSize(rc->size());
	
	_updateTimer.start(mainTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

void CameraController::adjustCameraToNextContextSize(const et::vec2& sz)
{
	_mainCamera.perspectiveProjection(QUARTER_PI, sz.aspect(), 1.0f, 8192.0f);
	_observerCamera.perspectiveProjection(QUARTER_PI, sz.aspect(), 1.0f, 8192.0f);
}

void CameraController::handlePointerDrag(const et::vec2& d)
{
	vec3 spherical = toSpherical(_mainCamera.direction());
	
	spherical.x = mix(spherical.x, spherical.x + d.x / DOUBLE_PI, 0.5f);
	
	spherical.y = clamp(mix(spherical.y, spherical.y - d.y / DOUBLE_PI, 0.5f),
		DEG_1 - HALF_PI, HALF_PI - DEG_1);
	
	_mainCamera.setDirection(fromSpherical(spherical.y, spherical.x));
}

void CameraController::handlePressedKey(size_t key)
{
	switch (key)
	{
		case ET_KEY_W:
			_movements.x = 1.0f;
			break;
		case ET_KEY_S:
			_movements.x = -1.0f;
			break;
		case ET_KEY_A:
			_movements.y = -1.0f;
			break;
		case ET_KEY_D:
			_movements.y = 1.0f;
			break;
		case ET_KEY_SHIFT:
			_boostEnabled = true;
			break;
			
		default:
			log::info("Unknown key pressed: %lu", key);
			break;
	}
}

void CameraController::handleReleasedKey(size_t key)
{
	if ((key == ET_KEY_W) || (key == ET_KEY_S))
	{
		_movements.x = 0.0f;
	}
	else if ((key == ET_KEY_A) || (key == ET_KEY_D))
	{
		_movements.y = 0.0f;
	}
	else if (key == ET_KEY_SHIFT)
	{
		_boostEnabled = false;
	}
	else
	{
		log::info("Unknown key released: %lu", key);
	}
}
