/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/sampler.h>

namespace et
{

struct MetalState;
struct MetalNativeSampler;
	
class MetalSamplerPrivate;
class MetalSampler : public Sampler
{
public:
	ET_DECLARE_POINTER(MetalSampler);

public:
	MetalSampler(MetalState&, const Description&);
	~MetalSampler();

	const MetalNativeSampler& nativeSampler() const;

private:
	ET_DECLARE_PIMPL(MetalSampler, 64);
};

}
