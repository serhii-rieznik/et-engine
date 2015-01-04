/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/geometry.h>
#include <et/app/application.h>
#include <et/input/gestures.h>

using namespace et;

const float GesturesRecognizer::defaultClickTemporalThreshold = 0.25f;
const float GesturesRecognizer::defaultClickSpatialTreshold = std::sqrt(50.0f);
const float GesturesRecognizer::defaultHoldTemporalThreshold = 1.0f;

GesturesRecognizer::GesturesRecognizer(bool automaticMode) : InputHandler(automaticMode),
	_clickTemporalThreshold(defaultClickTemporalThreshold),
	_clickSpatialThreshold(defaultClickSpatialTreshold),
	_holdTemporalThreshold(defaultHoldTemporalThreshold)
{
}

void GesturesRecognizer::handlePointersMovement()
{
    if (_pointers.size() == 2)
    {
        vec2 currentPositions[2];
        vec2 previousPositions[2];
		
        size_t index = 0;
		for (auto& i : _pointers)
        {
            currentPositions[index] = i.second.current.normalizedPos;
            previousPositions[index] = i.second.previous.normalizedPos;
			i.second.moved = false;
			++index;
        }
		
		vec2 dir0 = currentPositions[0] - previousPositions[0];
		vec2 dir1 = currentPositions[1] - previousPositions[1];
		vec2 center = 0.5f * (previousPositions[0] + previousPositions[1]);
		
		RecognizedGesture gesture = _gesture;
		if (gesture == RecognizedGesture_NoGesture)
		{
			vec2 nDir0 = normalize(dir0);
			vec2 nDir1 = normalize(dir1);
			float direction = dot(nDir0, nDir1);
			if (direction < -0.5f)
			{
				bool catchZoom = (_recognizedGestures & RecognizedGesture_Zoom) == RecognizedGesture_Zoom;
				bool catchRotate = (_recognizedGestures & RecognizedGesture_Rotate) == RecognizedGesture_Rotate;

				if (catchZoom && catchRotate)
				{
					float aspect = std::abs(dot(nDir0, normalize(previousPositions[0] - center))) +
						std::abs(dot(nDir1, normalize(previousPositions[1] - center)));
					gesture = (aspect > SQRT_3) ? RecognizedGesture_Zoom : RecognizedGesture_Rotate;
				}
				else if (catchZoom)
				{
					gesture = RecognizedGesture_Zoom;
				}
				else if (catchRotate)
				{
					gesture = RecognizedGesture_Rotate;
				}
			}
			else if ((direction > 0.5f) && (_recognizedGestures & RecognizedGesture_Swipe))
			{
				gesture = RecognizedGesture_Swipe;
			}
		}
		
		switch (gesture)
		{
			case RecognizedGesture_Zoom:
			{
				float zoomValue = (currentPositions[0] - currentPositions[1]).length() /
					(previousPositions[0] - previousPositions[1]).length();
				
				zoomAroundPoint.invoke(zoomValue, center);
				zoom.invoke(zoomValue);
				break;
			}
				
			case RecognizedGesture_Rotate:
			{
				vec2 centerToCurrent0 = currentPositions[0] - center;
				vec2 centerToCurrent1 = currentPositions[1] - center;
				float angle0 = std::atan2(dir0.length(), centerToCurrent0.length());
				float angle1 = std::atan2(dir1.length(), centerToCurrent1.length());
				float angleValue = (outerProduct(centerToCurrent0, dir0) < 0.0f ? 0.5f : -0.5f) * (angle0 + angle1);
				rotate.invoke(angleValue);
				break;
			}
				
			case RecognizedGesture_Swipe:
			{
				swipe.invoke(0.5f * (dir0 + dir1), 2);
				break;
			}

			default:
				break;
		}
		
		if (_lockGestures)
			_gesture = gesture;

    }
}

void GesturesRecognizer::cancelWaitingForClicks()
{
	_shouldPerformClick = false;
	_shouldPerformDoubleClick = false;
	_singlePointer = PointerInputInfo();
	cancelUpdates();
}

void GesturesRecognizer::update(float t)
{
	if (t - _singlePointer.timestamp >= _clickTemporalThreshold)
	{
		click.invoke(_singlePointer);
		cancelWaitingForClicks();
	}
}

void GesturesRecognizer::onPointerPressed(et::PointerInputInfo pi)
{
	pointerPressed.invoke(pi);

	_pointers[pi.id] = PointersInputDelta(pi, pi);
	
	pressed.invoke(pi.normalizedPos, pi.type);
	
	if (_pointers.size() == 1)
	{
		if (_shouldPerformClick)
		{
			if ((pi.pos - _singlePointer.pos).dotSelf() <= sqr(_clickSpatialThreshold))
			{
				_shouldPerformClick = false;
				_shouldPerformDoubleClick = true;
			}
			else
			{
				click.invoke(_singlePointer);
				
				_singlePointer = pi;
				_shouldPerformClick = true;
				_shouldPerformDoubleClick = false;
			}
		}
		else
		{
			_shouldPerformClick = true;
			_singlePointer = pi;
		}
		
		_singlePointer.id = pi.id;
	}
	else
	{
		_singlePointer = PointerInputInfo();
		cancelWaitingForClicks();
	}
}

void GesturesRecognizer::onPointerMoved(et::PointerInputInfo pi)
{
	pointerMoved.invoke(pi);
	
	if ((pi.id == 0) || (_pointers.count(pi.id) == 0)) return;

	if (_pointers.size() == 1)
	{
		bool hasPressedPointer = (_singlePointer.id != 0);
		bool shouldPerformMovement = !hasPressedPointer;
		
		if (hasPressedPointer && (pi.id == _singlePointer.id))
		{
			float len = (pi.pos - _singlePointer.pos).dotSelf();
			shouldPerformMovement = (len >= sqr(_clickSpatialThreshold));
		}
		
		if (shouldPerformMovement)
		{
			if (hasPressedPointer)
			{
				cancelWaitingForClicks();
				clickCancelled.invoke();
			}
			
			_pointers[pi.id].moved = true;
			_pointers[pi.id].previous = _pointers[pi.id].current;
			_pointers[pi.id].current = pi;
			
			const PointerInputInfo& pPrev = _pointers[pi.id].previous;
			const PointerInputInfo& pCurr = _pointers[pi.id].current; 

			vec2 offset = pCurr.normalizedPos - pPrev.normalizedPos;
			vec2 speed = offset / etMax(0.01f, pCurr.timestamp - pPrev.timestamp);
			
			moved.invoke(pi.normalizedPos, pi.type);
			
			if (pCurr.type == PointerType_General)
				dragWithGeneralPointer.invokeInMainRunLoop(speed, offset);
			
			drag.invoke(speed, pi.type);
		}
	}
	else
	{
		cancelWaitingForClicks();
		
		_pointers[pi.id].moved = true;
		_pointers[pi.id].previous = _pointers[pi.id].current;
		_pointers[pi.id].current = pi;
		
		bool allMoved = true;
		for (auto& p : _pointers)
		{
			if (!p.second.moved)
			{
				allMoved = false;
				break;
			}
		};
		
		if (allMoved)
			handlePointersMovement();
	}
}

void GesturesRecognizer::onPointerReleased(et::PointerInputInfo pi)
{
	pointerReleased.invoke(pi);

	_gesture = RecognizedGesture_NoGesture;
	_pointers.erase(pi.id);
	
	released.invoke(pi.normalizedPos, pi.type);
	
	if (pi.id == _singlePointer.id)
	{
		if ((pi.normalizedPos - _singlePointer.normalizedPos).dotSelf() > sqr(_clickSpatialThreshold))
		{
			cancelWaitingForClicks();
		}
		else if (_shouldPerformClick)
		{
			float dt = pi.timestamp - _singlePointer.timestamp;
			if ((dt > _clickTemporalThreshold))
			{
				click.invoke(_singlePointer);
				cancelWaitingForClicks();
			}
			else
			{
				startUpdates();
			}
		}
		else if (_shouldPerformDoubleClick)
		{
			doubleClick.invoke(_singlePointer);
			cancelWaitingForClicks();
		}
	}
}

void GesturesRecognizer::onPointerCancelled(et::PointerInputInfo pi)
{
	pointerCancelled.invoke(pi);
	
	_pointers.erase(pi.id);
	cancelled.invoke(pi.normalizedPos, pi.type);
	
	if (pi.id == _singlePointer.id)
		cancelWaitingForClicks();
}

void GesturesRecognizer::onPointerScrolled(et::PointerInputInfo i)
{
	pointerScrolled.invoke(i);
	scroll.invoke(i.scroll, i.origin);
}

void GesturesRecognizer::onGesturePerformed(GestureInputInfo i)
{
	if (i.mask & GestureTypeMask_Zoom)
		zoom.invoke(1.0f - i.values.z);

	if (i.mask & GestureTypeMask_Swipe)
		drag.invoke(i.values.xy(), PointerType_None);
	
	if (i.mask & GestureTypeMask_Rotate)
		rotate.invokeInMainRunLoop(i.values.w);
}

void GesturesRecognizer::setRecognizedGestures(size_t values)
{
	_recognizedGestures = values;
	
	if ((values & _gesture) == 0)
		_gesture = RecognizedGesture_NoGesture;
}
