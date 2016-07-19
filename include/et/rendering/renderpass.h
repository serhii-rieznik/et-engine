/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/renderbatch.h>

namespace et
{
	class RenderPass : public Shared
	{
	public:
		ET_DECLARE_POINTER(RenderPass)

		struct ConstructionInfo
		{
			struct ColorAttachment
			{
				FramebufferOperation loadOperation = FramebufferOperation::DontCare;
				FramebufferOperation storeOperation = FramebufferOperation::DontCare;
				vec4 clearColor = vec4(0.0f);
			} colorAttachment;

			struct DepthAttachment
			{
				FramebufferOperation loadOperation = FramebufferOperation::DontCare;
				FramebufferOperation storeOperation = FramebufferOperation::DontCare;
				float clearDepth = 1.0f;
			} depthAttachment;

            vec3 defaultLightPosition;
			Camera camera;
		};
		
	public:
		RenderPass(const ConstructionInfo&);
		~RenderPass();
		
		void pushRenderBatch(RenderBatch::Pointer);
		
		Vector<RenderBatch::Pointer>& renderBatches();
		const Vector<RenderBatch::Pointer>& renderBatches() const;
		
        const ConstructionInfo& info() const
            { return _info; }
		
	private:
        ConstructionInfo _info;
		Vector<RenderBatch::Pointer> _renderBatches;
	};
}
