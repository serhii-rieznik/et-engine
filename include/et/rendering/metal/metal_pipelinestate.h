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
	struct MetalNativeEncoder;
    
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
		const MetalNativeBuffer& variablesBuffer() const;

		void bind(MetalNativeEncoder&, Material::Pointer);

		template <typename T>
		void setProgramVariable(const String& name, const T& t)
			{ uploadProgramVariable(name, &t, sizeof(T)); }

	private:
		void uploadProgramVariable(const String& name, const void* ptr, uint32_t size);

	private:
		ET_DECLARE_PIMPL(MetalPipelineState, 256);
	};
}
