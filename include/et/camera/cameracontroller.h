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
		CameraController(Camera& cam, bool autoConnectToInput);
		
		Camera& camera()
			{ return _camera; }
		
		const Camera& camera() const
			{ return _camera; }
		
		void setPointerEventsEnabled(bool);
		void setKeyboardEventsEnabled(bool);
		
	public:
		virtual void startUpdates();
		virtual void cancelUpdates();
		
		virtual void synchronize(const Camera&) { }
		
		virtual void onKeyPressed(size_t) { }
		virtual void onKeyReleased(size_t) { }
		virtual void onPointerPressed(PointerInputInfo) { }
		virtual void onPointerMoved(PointerInputInfo) { }
		virtual void onPointerReleased(PointerInputInfo) { }
		virtual void onPointerCancelled(PointerInputInfo) { }
		virtual void onPointerScrolled(PointerInputInfo) { }
		
	protected:
		virtual void update(float) { }
		
	private:
		Camera& _camera;
		NotifyTimer _updateTimer;
		float _lastUpdateTime = 0.0f;
		bool _pointerEventsEnabled = false;
		bool _keyboardEventsEnabled = false;
	};
}
