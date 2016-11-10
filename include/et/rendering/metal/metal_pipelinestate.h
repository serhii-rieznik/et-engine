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
    class MetalRenderer;
	class MetalPipelineStatePrivate;
	class MetalPipelineState : public PipelineState
	{
	public:
		ET_DECLARE_POINTER(MetalPipelineState);

	public:
		MetalPipelineState(MetalRenderer*, MetalState&);
		~MetalPipelineState();

		void build() override;
        
        const MetalNativePipelineState& nativeState() const;

		void bind(MetalNativeEncoder&, MaterialInstance::Pointer);

		template <typename T>
		void setMaterialVariable(const String& name, const T& t)
			{ uploadMaterialVariable(name, &t, sizeof(T)); }

		template <typename T>
		void setObjectVariable(const String& name, const T& t)
			{ uploadObjectVariable(name, &t, sizeof(T)); }

	private:
		void buildRequiredLayout(VertexDeclaration& decl);
		void uploadObjectVariable(const String& name, const void* ptr, uint32_t size);
		void uploadMaterialVariable(const String& name, const void* ptr, uint32_t size);
		void buildReflection();

	private:
		ET_DECLARE_PIMPL(MetalPipelineState, 256);
	};
}
