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
	class RenderPass : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(RenderPass)
		
		struct ConstructionInfo
		{
			et::Camera::Pointer camera;
		};
		
	public:
		RenderPass(const ConstructionInfo&);
		~RenderPass();
		
		void pushRenderBatch(RenderBatch::Pointer);
		
		std::vector<RenderBatch::Pointer>& renderBatches();
		const std::vector<RenderBatch::Pointer>& renderBatches() const;
		
		et::Camera::Pointer camera() const;
		
	private:
		et::Camera::Pointer _camera;
		std::vector<RenderBatch::Pointer> _renderBatches;
	};
}
