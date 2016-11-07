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
	class VulkanState;
	class VulkanRenderer;
	class VulkanNativePipeline;
	class VulkanNativeRenderPass;
	class VulkanPipelineStatePrivate;
	class VulkanPipelineState : public PipelineState
	{
	public:
		ET_DECLARE_POINTER(VulkanPipelineState);

	public:
		VulkanPipelineState(VulkanRenderer*, VulkanState&);
		~VulkanPipelineState();

		const VulkanNativePipeline& nativePipeline() const;

		void build() override;
		void bind(VulkanNativeRenderPass&, MaterialInstance::Pointer&);

	private:
		ET_DECLARE_PIMPL(VulkanPipelineState, 256);
	};
}
