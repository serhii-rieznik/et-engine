/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/raytraceobjects.h>
#include <et/imaging/texturedescription.h>

namespace et
{
namespace rt
{

class ImagePrivate;
class Image : public Object
{
public:
	ET_DECLARE_POINTER(Image);

public:
	Image(const TextureDescription::Pointer);
	~Image() override;

	float4 pointSample(uint32_t x, uint32_t y);
	float4 sample(float u, float v);

	float4 quirectangularSample(float phi, float theta);

private:
	ET_DECLARE_PIMPL(Image, 256);
};

}
}
