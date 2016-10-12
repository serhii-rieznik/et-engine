/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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
		
		struct DragGesture
		{
			vec2 delta = vec2(0.0f);
			vec2 velocity = vec2(0.0f);
			PointerType pointerType = PointerType_None;
		};
		
	public:
		GesturesRecognizer(bool automaticMode = true);
		
		size_t pointersCount() const
			{ return _pointers.size(); }
		
		void setShouldLockGestures(bool lock)
			{ _lockGestures = lock; }
		
		void setRecognizedGestures(size_t);

		void cancelRecognition();

	public:
		float clickTemporalThreshold() const
			{ return _clickTemporalThreshold; }

		void setClickTemporalThreshold(float clickTemporalThreshold)
			{ _clickTemporalThreshold = clickTemporalThreshold; }

		float clickSpatialThreshold() const
			{ return _clickSpatialThreshold; }

		void setClickSpatialThreshold(float clickSpatialThreshold)
			{ _clickSpatialThreshold = clickSpatialThreshold; }

		float holdTemporalThreshold() const
			{ return _holdTemporalThreshold; }

		void setHoldTemporalThreshold(float holdTemporalThreshold)
			{ _holdTemporalThreshold = holdTemporalThreshold; }

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
		 */
		ET_DECLARE_EVENT1(drag, const DragGesture&)
				
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
		ET_DENY_COPY(GesturesRecognizer);
		
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
        using PointersInputDeltaMap = Map<size_t, PointersInputDelta>;

	private:
		PointersInputDeltaMap _pointers;
		PointerInputInfo _singlePointer;
		size_t _recognizedGestures = RecognizedGesture_All;
		RecognizedGesture _gesture = RecognizedGesture_NoGesture;
		float _clickTemporalThreshold = defaultClickTemporalThreshold;
		float _clickSpatialThreshold = defaultClickSpatialTreshold;
		float _holdTemporalThreshold = defaultHoldTemporalThreshold;
		bool _shouldPerformClick = false;
		bool _shouldPerformDoubleClick = false;
		bool _lockGestures = true;
	};
}
