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
	typedef std::function<void(const et::vec2i& location, const et::vec4& color)> OutputFunction;
	
	void raytrace(const RaytraceScene&, const et::vec2i& imageSize,
		const et::vec2i& origin, const et::vec2i& size, OutputFunction);
}
