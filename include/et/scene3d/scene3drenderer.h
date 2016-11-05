/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>
#include <et/scene3d/scene3d.h>

namespace et
{
	namespace s3d
	{		
		class Renderer : public FlagsHolder
		{
		public:
			ET_DECLARE_POINTER(Renderer);
			
			enum : uint64_t
			{
				RenderMeshes = 0x01,
				RenderDebugObjects = 0x02,
				Wireframe = 0x04,
				
				RenderAll = RenderMeshes | RenderDebugObjects
			};
			
		public:
			Renderer();

			void render(RenderInterface::Pointer, const Scene&, Camera::Pointer);
			void renderTransformedBoundingBox(RenderPass::Pointer, const BoundingBox&, const mat4&);
			
		private:
			void renderMeshList(RenderPass::Pointer, const s3d::BaseElement::List&);
			
		private:
			using BatchFromMesh = std::pair<et::RenderBatch::Pointer, et::s3d::Mesh::Pointer>;
			using BatchMap = std::map<uint64_t, Vector<BatchFromMesh>, std::greater<uint64_t>>;

			RenderPass::Pointer _mainPass;
			RenderBatch::Pointer _bboxBatch;
			BatchMap _latestBatches;
		};
	}
}
