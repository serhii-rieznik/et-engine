/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/geometry/geometry.h>
#include <et/app/application.h>
#include <et/input/gestures.h>

using namespace et;

GesturesRecognizer::GesturesRecognizer(bool automaticMode) : InputHandler(automaticMode),
	_clickThreshold(0.2f), _doubleClickThreshold(0.25f), _holdThreshold(1.0f),
	_singlePointerType(0), _actualTime(0.0f), _clickStartTime(0.0f), _expectClick(false),
	_expectDoubleClick(false), _clickTimeoutActive(false)
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
		
		vec2 dir1 = currentPositions[0] - previousPositions[0];
		vec2 dir2 = currentPositions[1] - previousPositions[1];
		float direction = dot(normalize(dir1), normalize(dir2));

		if (direction < -0.5f) // handle zoom gesture
		{
			float currentDistance = (currentPositions[0] - currentPositions[1]).length();
			float previousDistance = (previousPositions[0] - previousPositions[1]).length();
			float zoomValue = currentDistance / previousDistance;
			
			vec2 center = 0.5f * (previousPositions[0] + previousPositions[1]);
			float angle1 = outerProduct(dir1, previousPositions[0] - center);
			float angle2 = outerProduct(dir2, previousPositions[1] - center);
			rotate.invoke(0.5f * (angle1 + angle2));
			zoomAroundPoint.invoke(zoomValue, center);
			zoom.invoke(zoomValue);
		}
		else if (direction > 0.5f) // handle swipe gesture
		{
			swipe.invoke(0.5f * (dir1 + dir2), 2);
		}
    }
    else 
    {
        
    }
}

void GesturesRecognizer::onPointerPressed(et::PointerInputInfo pi)
{
	pointerPressed.invoke(pi);

	_pointers[pi.id] = PointersInputDelta(pi, pi);
	pressed.invoke(pi.normalizedPos, pi.type);
	
	if (_pointers.size() == 1)
	{
		_singlePointerType = pi.type;
		_singlePointerPosition = pi.normalizedPos;
		float currentTime = mainTimerPool()->actualTime();
		float dt = currentTime - _actualTime;
		
		_clickTimeoutActive = false;
		_expectClick = dt > _doubleClickThreshold;
		_expectDoubleClick = !_expectClick;

		if (_expectClick)
			_clickStartTime = currentTime;
		
		startWaitingForClicks();
	}
	else
	{
		cancelWaitingForClicks();
	}
}

void GesturesRecognizer::onPointerMoved(et::PointerInputInfo pi)
{
	pointerMoved.invoke(pi);
	if (_pointers.count(pi.id) == 0) return;

	_pointers[pi.id].moved = true;
	_pointers[pi.id].previous = _pointers[pi.id].current;
	_pointers[pi.id].current = pi;

	if (_pointers.size() == 1)
	{
		const PointerInputInfo& pPrev = _pointers[pi.id].previous; 
		const PointerInputInfo& pCurr = _pointers[pi.id].current; 

		vec2 offset = pCurr.normalizedPos - pPrev.normalizedPos;
		vec2 speed = offset / etMax(0.01f, pCurr.timestamp - pPrev.timestamp);
		
		moved.invoke(pi.normalizedPos, pi.type);
		
		if (pCurr.type == PointerType_General)
			dragWithGeneralPointer.invokeInMainRunLoop(speed, offset);
		
		drag.invoke(speed, pi.type);
	}
	else
	{
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
	
	cancelWaitingForClicks();
}

void GesturesRecognizer::onPointerReleased(et::PointerInputInfo pi)
{
	pointerReleased.invoke(pi);

	_pointers.erase(pi.id);
	released.invoke(pi.normalizedPos, pi.type);
	stopWaitingForClicks();
}

void GesturesRecognizer::onPointerCancelled(et::PointerInputInfo pi)
{
	pointerCancelled.invoke(pi);

	_pointers.erase(pi.id);
	cancelled.invoke(pi.normalizedPos, pi.type);
	cancelWaitingForClicks();
}

void GesturesRecognizer::onPointerScrolled(et::PointerInputInfo i)
{
	pointerScrolled.invoke(i);
	scroll.invoke(i.scroll, i.origin);
}

void GesturesRecognizer::update(float t)
{
	if (_clickTimeoutActive && (t >= _clickStartTime + _clickThreshold))
	{
		click.invoke(_singlePointerPosition, _singlePointerType);
		cancelUpdates();
		_clickTimeoutActive = false;
	}
	
	_actualTime = t;
}

void GesturesRecognizer::startWaitingForClicks()
{
	startUpdates();
}

void GesturesRecognizer::stopWaitingForClicks()
{
	if (_expectDoubleClick)
	{
		doubleClick.invoke(_singlePointerPosition, _singlePointerType);
		_expectDoubleClick = false;
		_expectClick = false;
		cancelUpdates();
	}
	else if (_expectClick)
	{
		_clickTimeoutActive = true;
	}
}

void GesturesRecognizer::cancelWaitingForClicks()
{
	_expectClick = false;
	_expectDoubleClick = false;
	cancelUpdates();
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
