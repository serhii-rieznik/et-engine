/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/objects/light.h>
#include <et/rendering/interface/compute.h>
#include <et/rendering/base/rendering.h>
#include <et/rendering/base/constantbuffer.h>
#include <et/rendering/base/renderbatch.h>

namespace et
{

struct RenderTarget
{
	Texture::Pointer texture;
	FramebufferOperation loadOperation = FramebufferOperation::DontCare;
	FramebufferOperation storeOperation = FramebufferOperation::DontCare;
	vec4 clearValue = vec4(1.0f);
	bool useDefaultRenderTarget = true;
	bool enabled = false;
};

struct RenderSubpass
{
	uint32_t layer = 0;
	uint32_t level = 0;

	RenderSubpass() = default;
	RenderSubpass(uint32_t aLayer, uint32_t aLevel) : 
		layer(aLayer), level(aLevel) { }
};

struct RenderPassBeginInfo
{
	Vector<RenderSubpass> subpasses;

	RenderPassBeginInfo() = default;

	RenderPassBeginInfo(uint32_t l, uint32_t m) : 
		subpasses(1, { l, m }) { }

	static const RenderPassBeginInfo& singlePass()
	{
		static const RenderPassBeginInfo singlePassObject(0, 0);
		return singlePassObject;
	}
};

struct CopyDescriptor
{
	uint32_t levelFrom = 0;
	uint32_t levelTo = 0;
	uint32_t layerFrom = 0;
	uint32_t layerTo = 0;
	vec3i offsetFrom = vec3i(0, 0, 0);
	vec3i offsetTo = vec3i(0, 0, 0);
	vec3i size = vec3i(0, 0, 0);
};

class RenderInterface;
class RenderPass : public Shared
{
public:
	ET_DECLARE_POINTER(RenderPass);

	struct ConstructionInfo
	{
		std::string name;
		RenderTarget color[MaxRenderTargets];
		RenderTarget depth;
		uint32_t priority = RenderPassPriority::Default;
		bool enableDepthBias = false;
	};

	static const std::string kPassNameDefault;
	static const std::string kPassNameUI;
	static const std::string kPassNameDepth;

public:
	RenderPass(RenderInterface*, const ConstructionInfo&);
	virtual ~RenderPass();

	virtual void begin(const RenderPassBeginInfo& info) = 0;
	virtual void pushRenderBatch(const RenderBatch::Pointer&) = 0;
	virtual void pushImageBarrier(const Texture::Pointer&, const ResourceBarrier&) = 0;
	virtual void copyImage(const Texture::Pointer&, const Texture::Pointer&, const CopyDescriptor&) = 0;
	virtual void dispatchCompute(const Compute::Pointer&, const vec3i&) = 0;
	virtual void nextSubpass() = 0;
	virtual void end() = 0;

	const ConstructionInfo& info() const;

	void setSharedTexture(MaterialTexture, const Texture::Pointer&, const Sampler::Pointer&);
	
	template <class T>
	void setSharedVariable(ObjectVariable var, const T& value);

	template <class T>
	bool loadSharedVariable(ObjectVariable var, T& value);

	void loadSharedVariablesFromCamera(const Camera::Pointer&);
	void loadSharedVariablesFromLight(const Light::Pointer&);

	uint64_t identifier() const;

	static ConstructionInfo renderTargetPassInfo(const std::string& name, const Texture::Pointer&);

protected:
	using SharedTexturesSet = std::map<MaterialTexture, std::pair<Texture::Pointer, Sampler::Pointer>>;
	const SharedTexturesSet& sharedTextures() const { return _sharedTextures; }
	const VariablesHolder& sharedVariables() const { return _sharedVariables; }

private:
	ConstructionInfo _info;
	SharedTexturesSet _sharedTextures;
	VariablesHolder _sharedVariables;
};

template <class T>
inline void RenderPass::setSharedVariable(ObjectVariable var, const T& value)
{
	_sharedVariables[static_cast<uint32_t>(var)] = value;
}

template <class T>
inline bool RenderPass::loadSharedVariable(ObjectVariable var, T& value)
{
	bool available = _sharedVariables[static_cast<uint32_t>(var)].isSet();
	if (available)
	{
		value = _sharedVariables[static_cast<uint32_t>(var)].as<T>();
	}
	return available;
}

}
