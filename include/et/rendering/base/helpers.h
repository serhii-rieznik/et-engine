/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/renderbatch.h>

namespace et {

namespace renderhelper {

void init(RenderInterface::Pointer&);
void release();

enum class QuadType : uint32_t
{
	Default,
	Fullscreen
};

RenderBatch::Pointer createQuadBatch(Material::Pointer = Material::Pointer(), QuadType type = QuadType::Fullscreen);
RenderBatch::Pointer createQuadBatch(const std::string&, const Texture::Pointer&, const Material::Pointer& = Material::Pointer(), QuadType type = QuadType::Fullscreen);
RenderBatch::Pointer createQuadBatch(const std::string&, const Texture::Pointer&, const Material::Pointer&, const ResourceRange&, QuadType type = QuadType::Fullscreen);

};

}
