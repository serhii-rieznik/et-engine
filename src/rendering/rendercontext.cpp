/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>

using namespace et;

void RenderContext::onFPSTimerExpired(NotifyTimer*)
{
	if (_info.averageFramePerSecond > 0)
	{
		_info.averageDIPPerSecond /= _info.averageFramePerSecond;
		_info.averagePolygonsPerSecond /= _info.averageFramePerSecond;
		_info.averageFrameTimeInMicroseconds /= _info.averageFramePerSecond;
	}
	
	renderingInfoUpdated.invoke(_info);
	
	_info.averageFramePerSecond = 0;
	_info.averageDIPPerSecond = 0;
	_info.averagePolygonsPerSecond = 0;
	_info.averageFrameTimeInMicroseconds = 0;
}

void RenderContext::resized(const vec2i& sz)
{
	updateScreenScale(sz);
	
	_renderState.setMainViewportSize(sz);
	
	if (_app->running())
		_app->contextResized(sz);
}

void RenderContext::updateScreenScale(const vec2i& screenSize)
{
	if (_screenScaleFactorSet && _params.lockScreenScaleToInitial) return;
	
	int maxDimension = std::max(screenSize.x, screenSize.y);
	int maxBaseSize = std::max(_params.contextBaseSize.x, _params.contextBaseSize.y);
	
	size_t newScale = static_cast<size_t>((maxDimension - 1) / (3 * maxBaseSize / 2) + 1);
	if (newScale == _screenScaleFactor) return;
	
	_screenScaleFactor = std::min(_maxScreenScaleFactor, newScale);
	_screenScaleFactorSet = true;
	
	screenScaleFactorChanged.invoke(_screenScaleFactor);
}
