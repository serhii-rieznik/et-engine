/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/input/input.h>

namespace et
{
	class GesturesRecognizer : private InputHandler, private TimedObject
	{
	public:
		GesturesRecognizer(bool automaticMode = true);
		
		size_t pointersCount() const
			{ return _pointers.size(); }

	public:
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, clickThreshold, setClickThreshold)
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, doubleClickTemporalThreshold, setDoubleClickTemporalThreshold)
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, doubleClickSpatialThreshold, setDoubleClickSpatialThreshold)
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, holdThreshold, setHoldThreshold)

	public:
		ET_DECLARE_EVENT1(rotate, float)
		
		ET_DECLARE_EVENT1(zoom, float)
		ET_DECLARE_EVENT2(zoomAroundPoint, float, vec2)
		
		ET_DECLARE_EVENT2(scroll, vec2, PointerOrigin)

		/**
		 * Multiple pointers swipe gesture
		 * first parameter - direction in normalized coordinates
		 * second paramter - number of pointers
		 */
		ET_DECLARE_EVENT2(swipe, vec2, size_t)
		
		/*
		 * Drag gesture
		 * first parameter - velocity
		 */
		ET_DECLARE_EVENT2(drag, vec2, PointerType)
		
		/*
		 * Drag gesture
		 * first parameter - velocity
		 * second parameter - offset
		 */
		ET_DECLARE_EVENT2(dragWithGeneralPointer, vec2, vec2)
		
		ET_DECLARE_EVENT2(pressed, vec2, PointerType)
		ET_DECLARE_EVENT2(moved, vec2, PointerType)
		ET_DECLARE_EVENT2(released, vec2, PointerType)
		ET_DECLARE_EVENT2(cancelled, vec2, PointerType)
		
		ET_DECLARE_EVENT2(click, vec2, PointerType)
		ET_DECLARE_EVENT2(doubleClick, vec2, PointerType)

		ET_DECLARE_EVENT0(hold)

		ET_DECLARE_EVENT1(pointerPressed, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerMoved, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerReleased, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerCancelled, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerScrolled, PointerInputInfo)

	public:
		void onPointerPressed(PointerInputInfo);
		void onPointerMoved(PointerInputInfo);
		void onPointerReleased(PointerInputInfo);
        void onPointerCancelled(PointerInputInfo);
		void onPointerScrolled(PointerInputInfo);

		void onGesturePerformed(GestureInputInfo);

	private:
		void update(float);
		void handlePointersMovement();
		void startWaitingForClicks();
		void stopWaitingForClicks();
		void cancelWaitingForClicks();

		GesturesRecognizer(const GesturesRecognizer&) : InputHandler(false)
			{ }
		
		GesturesRecognizer& operator = (const GesturesRecognizer&)
			{ return *this; }

	private:
		struct PointersInputDelta
		{
			PointerInputInfo current;
			PointerInputInfo previous;
			bool moved;

			PointersInputDelta() : moved(false)
				{ } 

			PointersInputDelta(const PointerInputInfo& c, const PointerInputInfo& p) : 
				current(c), previous(p), moved(false) { }
		};
        typedef std::map<size_t, PointersInputDelta> PointersInputDeltaMap;

		PointersInputDeltaMap _pointers;
		PointerType _singlePointerType;
		vec2 _singlePointerPosition;
		float _actualTime;
		float _clickStartTime;
		bool _expectClick;
		bool _expectDoubleClick;
		bool _clickTimeoutActive;
	};
}