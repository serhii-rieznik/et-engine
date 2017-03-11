/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/objects/light.h>
#include <et/rendering/base/rendering.h>
#include <et/rendering/base/constantbuffer.h>
#include <et/rendering/base/renderbatch.h>

namespace et
{

struct RenderTarget
{
	Texture::Pointer texture;
	FramebufferOperation loadOperation = FramebufferOperation::Clear;
	FramebufferOperation storeOperation = FramebufferOperation::Store;
	vec4 clearValue = vec4(1.0f);
	bool useDefaultRenderTarget = true;
	bool enabled = false;
};

class RenderInterface;
class RenderPass : public Shared
{
public:
	ET_DECLARE_POINTER(RenderPass);

	struct ConstructionInfo
	{
		RenderTarget color[MaxRenderTargets];
		RenderTarget depth;
		Camera::Pointer camera;
		Light::Pointer light;
		float depthBias = 0.0f;
		float depthSlope = 0.0f;
		uint32_t priority = RenderPassPriority::Default;
		std::string name;
	};

	struct BeginInfo
	{
		uint32_t layerIndex = 0;
		uint32_t mipLevel = 0; // TODO : handle

		BeginInfo() = default;
		BeginInfo(uint32_t l) : layerIndex(l) { }
		BeginInfo(uint32_t l, uint32_t m) : layerIndex(l), mipLevel(m) { }
	};

	struct Variables
	{
		mat4 viewProjection;
		mat4 projection;
		mat4 view;
		mat4 inverseViewProjection;
		mat4 inverseProjection;
		mat4 inverseView;
		vec4 cameraPosition;
		vec4 cameraDirection;
		vec4 cameraUp;
		vec4 lightPosition;
		mat4 lightProjection;
	};

	static const std::string kPassNameForward;
	static const std::string kPassNameUI;
	static const std::string kPassNameDepth;

public:
	RenderPass(RenderInterface* renderer, const ConstructionInfo& info);
	virtual ~RenderPass();

	virtual void begin(const BeginInfo&) = 0;
	virtual void pushRenderBatch(const RenderBatch::Pointer&) = 0;
	virtual void end() = 0;

	void executeSingleRenderBatch(const RenderBatch::Pointer&, const BeginInfo&);

	const ConstructionInfo& info() const;
	ConstantBuffer& dynamicConstantBuffer();

	void setCamera(const Camera::Pointer& cam);
	void setLightCamera(const Camera::Pointer& cam);

	Camera::Pointer& camera();
	const Camera::Pointer& camera() const;

	void setSharedTexture(MaterialTexture, const Texture::Pointer&, const Sampler::Pointer&);

	uint64_t identifier() const;

protected:
	using SharedTexturesSet = std::map<MaterialTexture, std::pair<Texture::Pointer, Sampler::Pointer>>;
	const SharedTexturesSet& sharedTextures() const { return _sharedTextures; }

private:
	RenderInterface* _renderer = nullptr;
	ConstructionInfo _info;
	ConstantBuffer _dynamicConstantBuffer;
	SharedTexturesSet _sharedTextures;
};

}
