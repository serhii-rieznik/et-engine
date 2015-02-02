//
//  Raytracer.h
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include "RaytraceScene.h"

namespace rt
{
	struct RenderRect
	{
		et::recti r;
		size_t priority = 0;
		
		RenderRect(const et::recti& a, size_t p) :
			r(a), priority(p) { }
	};
	
	typedef std::vector<RenderRect> RenderRects;
	
	typedef std::function<void(const et::vec2i& location, const et::vec4& color)> OutputFunction;
	
	RenderRects estimateRenderRects(const RaytraceScene&, const et::vec2i& imageSize, bool preview);
	
	void raytrace(const RaytraceScene&, const et::vec2i& imageSize,
		const et::vec2i& origin, const et::vec2i& size, OutputFunction);
	
	void raytracePreview(const RaytraceScene&, const et::vec2i& imageSize,
		const et::vec2i& origin, const et::vec2i& size, OutputFunction);
}
