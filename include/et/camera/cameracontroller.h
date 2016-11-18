/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/input/input.h>
#include <et/camera/camera.h>
#include <et/core/notifytimer.h>

namespace et
{
class CameraController : public et::Shared, public EventReceiver
{
public:
	ET_DECLARE_POINTER(CameraController);

public:
	CameraController(Camera::Pointer cam, bool autoConnectToInput);

	Camera::Pointer camera()
	{
		return _camera;
	}

	const Camera::Pointer camera() const
	{
		return _camera;
	}

	void setPointerEventsEnabled(bool);
	void setKeyboardEventsEnabled(bool);

	void connectPointerEvents();
	void disconnectPointerEvents();
	void connectKeyboardEvents();
	void disconnectKeyboardEvents();

public:
	virtual void startUpdates();
	virtual void cancelUpdates();

	virtual void synchronize(const Camera::Pointer) {}

	virtual void onKeyPressed(uint32_t) {}
	virtual void onKeyReleased(uint32_t) {}
	virtual void onPointerPressed(PointerInputInfo) {}
	virtual void onPointerMoved(PointerInputInfo) {}
	virtual void onPointerReleased(PointerInputInfo) {}
	virtual void onPointerCancelled(PointerInputInfo) {}
	virtual void onPointerScrolled(PointerInputInfo) {}

protected:
	virtual void update(float) {}

	bool pointerEventsEnabled() const
	{
		return _pointerEventsEnabled;
	}

	bool keyboardEventsEnabled() const
	{
		return _keyboardEventsEnabled;
	}

private:
	Camera::Pointer _camera;
	NotifyTimer _updateTimer;
	float _lastUpdateTime = 0.0f;
	bool _pointerEventsEnabled = true;
	bool _keyboardEventsEnabled = true;
};
}
