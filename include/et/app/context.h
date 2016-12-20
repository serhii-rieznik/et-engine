/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>

namespace et
{
struct ContextOptions
{
	enum Style : uint32_t
	{
		Borderless = 0x00,
		Caption = 0x01,
		Sizable = 0x02,
		Hidden = 0x04
	};
	
	enum class SizeClass : uint32_t
	{
		Predefined,
		FillWorkarea,
		Fullscreen
	};
	
	vec2i size = vec2i(640, 480);
	vec2i minimumSize = vec2i(640, 480);
	uint32_t style = Style::Caption;
	SizeClass sizeClass = SizeClass::Predefined;
	bool keepAspectOnResize = false;
	bool supportsHighResolution = true;
};

struct ApplicationParameters
{
	ContextOptions context;
#if (ET_PLATFORM_WIN)
	RenderingAPI renderingAPI = RenderingAPI::Vulkan;
#elif (ET_PLATFORM_MAC)
	RenderingAPI renderingAPI = RenderingAPI::Metal;
#endif
	bool shouldSuspendOnDeactivate = false;
	bool shouldPreserveRenderContext = false;
};

struct PlatformDependentContext
{
	enum
	{
		ObjectsCount = 32
	};
	std::array<void*, ObjectsCount> objects;

	PlatformDependentContext()
	{
		std::fill(objects.begin(), objects.end(), nullptr);
	}
};
}
