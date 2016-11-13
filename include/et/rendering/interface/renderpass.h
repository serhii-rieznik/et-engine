/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/base/renderbatch.h>
#include <et/rendering/constantbuffer.h>

namespace et
{
class RenderInterface;
class RenderPass : public Shared
{
public:
	ET_DECLARE_POINTER(RenderPass);

	struct Target
	{
		// TODO : add render target
		FramebufferOperation colorLoadOperation = FramebufferOperation::DontCare;
		FramebufferOperation colorStoreOperation = FramebufferOperation::DontCare;
		FramebufferOperation depthLoadOperation = FramebufferOperation::DontCare;
		FramebufferOperation depthStoreOperation = FramebufferOperation::DontCare;
		vec4 clearColor = vec4(0.0f);
		float clearDepth = 1.0f;
	};

	struct ConstructionInfo
	{
		Target target;
		Camera::Pointer camera;
		Camera::Pointer light;
	};

	struct Variables
	{
		mat4 viewProjection;
		mat4 projection;
		mat4 view;
		vec4 cameraPosition;
		vec4 cameraDirection;
		vec4 cameraUp;
		vec4 lightPosition;
	};

public:
	RenderPass(RenderInterface* renderer, const ConstructionInfo& info);
	virtual ~RenderPass();

	virtual void begin() = 0;
	virtual void validateRenderBatch(RenderBatch::Pointer) = 0;
	virtual void pushRenderBatch(RenderBatch::Pointer) = 0;
	virtual void end() = 0;

	const ConstructionInfo& info() const;
	ConstantBuffer& dynamicConstantBuffer();

private:
	ConstructionInfo _info;
	ConstantBuffer _dynamicConstantBuffer;
};

inline RenderPass::RenderPass(RenderInterface* renderer, const ConstructionInfo& info) :
	_info(info)
{
	_dynamicConstantBuffer.init(renderer);
}

inline RenderPass::~RenderPass()
{
	_dynamicConstantBuffer.shutdown();
}

inline const RenderPass::ConstructionInfo& RenderPass::info() const
{
	return _info;
}

inline ConstantBuffer& RenderPass::dynamicConstantBuffer()
{
	return _dynamicConstantBuffer;
}

}
