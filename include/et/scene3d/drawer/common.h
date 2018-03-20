/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>
#include <et/scene3d/scene3d.h>

namespace et
{
namespace s3d
{

struct DrawerOptions
{
	float shadowmapScale = 1.0f;
	bool drawCubemapsDebug = false;
	bool drawShadowmap = false;
	bool rebuldEnvironmentProbe = false;
	bool rebuildLookupTexture = false;
	bool enableScreenSpaceShadows = false;
	bool enableScreenSpaceAO = true;
};

mat4 fullscreenBatchTransform(const vec2& viewport, const vec2& origin, const vec2& size);

}
}
