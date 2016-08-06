/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/pipelinestate.h>

namespace et
{
	struct MetalState;
    struct MetalNativePipelineState;
    
	class MetalPipelineStatePrivate;
	class MetalPipelineState : public PipelineState
	{
	public:
		ET_DECLARE_POINTER(MetalPipelineState);

	public:
		MetalPipelineState(MetalState&);
		~MetalPipelineState();

		void build() override;
        
        const MetalNativePipelineState& nativeState() const;

	private:
		ET_DECLARE_PIMPL(MetalPipelineState, 64);
	};
}
