/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_sampler.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalSamplerPrivate
{
public:
	MetalNativeSampler sampler;
};

MetalSampler::MetalSampler(MetalState& metal)
{
	ET_PIMPL_INIT(MetalSampler);

	MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
	desc.sAddressMode = MTLSamplerAddressModeRepeat;
	desc.tAddressMode = MTLSamplerAddressModeRepeat;
	_private->sampler.sampler = [metal.device newSamplerStateWithDescriptor:desc];

	ET_OBJC_RELEASE(desc);
}

MetalSampler::~MetalSampler()
{
	ET_PIMPL_FINALIZE(MetalSampler);
}

const MetalNativeSampler& MetalSampler::nativeSampler() const
{
	return _private->sampler;
}

}
