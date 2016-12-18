/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/image.h>

namespace et
{
namespace rt
{

class ImagePrivate
{
public:
};

Image::Image(const TextureDescription::Pointer) 
{
	ET_PIMPL_INIT(Image);
}

Image::~Image() 
{
	ET_PIMPL_FINALIZE(Image);
}

float4 Image::pointSample(uint32_t x, uint32_t y) 
{
	return float4(0.5);
}

rt::float4 Image::sample(float u, float v)
{
	return float4(0.333333f);
}

float4 Image::quirectangularSample(float phi, float theta)
{
	return float4(0.25);
}

}
}
