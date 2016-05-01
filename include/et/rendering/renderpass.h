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
			et::Camera camera;
            et::vec3 defaultLightPosition;
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
