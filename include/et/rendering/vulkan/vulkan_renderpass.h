/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderpass.h>

namespace et
{

class VulkanState;
class VulkanRenderer;
class VulkanNativeRenderPass;
class VulkanRenderPassPrivate;
class VulkanRenderPass : public RenderPass
{
public:
	ET_DECLARE_POINTER(VulkanRenderPass);

public:
	VulkanRenderPass(VulkanRenderer*, VulkanState&, const RenderPass::ConstructionInfo&);
	~VulkanRenderPass();

	const VulkanNativeRenderPass& nativeRenderPass() const;

	void begin(const RenderPassBeginInfo&) override;
	void pushRenderBatch(const MaterialInstance::Pointer&, const VertexStream::Pointer&, uint32_t, uint32_t) override;
	void pushImageBarrier(const Texture::Pointer&, const ResourceBarrier&) override;
	void copyImage(const Texture::Pointer&, const Texture::Pointer&, const CopyDescriptor&) override;
	void dispatchCompute(const Compute::Pointer&, const vec3i&) override;
	void endSubpass() override;
	void nextSubpass() override;
	void end() override;

	void debug() override;

	bool fillStatistics(uint64_t* buffer, RenderPassStatistics&);
	
private:
	ConstantBufferEntry::Pointer VulkanRenderPass::buildObjectVariables(const VulkanProgram::Pointer&);

private:
	ET_DECLARE_PIMPL(VulkanRenderPass, 384);
};
}
