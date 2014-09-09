/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
	int maxDimension = etMax(screenSize.x, screenSize.y);
	int maxBaseSize = etMax(_params.contextBaseSize.x, _params.contextBaseSize.y);
	
	size_t newScale = static_cast<size_t>((maxDimension - 1) / (3 * maxBaseSize / 2) + 1);
	if (newScale == _screenScaleFactor) return;
	
	_screenScaleFactor = etMin(_maxScreenScaleFactor, newScale);
	screenScaleFactorChanged.invoke(_screenScaleFactor);
}
