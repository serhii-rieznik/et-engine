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
	enum RecognizedGesture
	{
		RecognizedGesture_NoGesture = 0x00,
		RecognizedGesture_Zoom = 0x01,
		RecognizedGesture_Rotate = 0x02,
		RecognizedGesture_Swipe = 0x04,
		RecognizedGesture_All = RecognizedGesture_Zoom | RecognizedGesture_Rotate | RecognizedGesture_Swipe
	};
	
	class GesturesRecognizer : private InputHandler, private TimedObject
	{
	public:
		static const float defaultClickTemporalThreshold;
		static const float defaultClickSpatialTreshold;
		static const float defaultHoldTemporalThreshold;
		
	public:
		GesturesRecognizer(bool automaticMode = true);
		
		size_t pointersCount() const
			{ return _pointers.size(); }
		
		void setShouldLockGestures(bool lock)
			{ _lockGestures = lock; }
		
		void setRecognizedGestures(size_t);

	public:
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, clickTemporalThreshold, setClickTemporalThreshold)
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, clickSpatialThreshold, setClickSpatialThreshold)
				
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(float, holdTemporalThreshold, setHoldTemporalThreshold)

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
		
		ET_DECLARE_EVENT1(click, const PointerInputInfo&)
		ET_DECLARE_EVENT0(clickCancelled)
		
		ET_DECLARE_EVENT1(doubleClick, const PointerInputInfo&)

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
		ET_DENY_COPY(GesturesRecognizer)
		
		void update(float);
		void handlePointersMovement();
		void cancelWaitingForClicks();
		
	private:
		struct PointersInputDelta
		{
			PointerInputInfo current;
			PointerInputInfo previous;
			bool moved = false;

			PointersInputDelta() { }

			PointersInputDelta(const PointerInputInfo& c, const PointerInputInfo& p) : 
				current(c), previous(p) { }
		};
        typedef std::map<size_t, PointersInputDelta> PointersInputDeltaMap;

	private:
		PointersInputDeltaMap _pointers;
		PointerInputInfo _singlePointer;
		size_t _recognizedGestures = RecognizedGesture_All;
		RecognizedGesture _gesture = RecognizedGesture_NoGesture;
		bool _shouldPerformClick = false;
		bool _shouldPerformDoubleClick = false;
		bool _lockGestures = true;
	};
}
