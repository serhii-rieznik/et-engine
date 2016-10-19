/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderpass.h>
#include <et/scene3d/scene3d.h>

namespace et
{
	class Camera;
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
			void render(RenderContext*, const Scene&, const Camera&);

			void initDebugObjects(RenderContext*);
			void renderTransformedBoundingBox(RenderPass::Pointer, const BoundingBox&, const mat4&);
			
		private:
			void renderMeshList(RenderPass::Pointer, const s3d::BaseElement::List&);
			
		private:
			using BatchFromMesh = std::pair<et::RenderBatch::Pointer, et::s3d::Mesh::Pointer>;
			std::map<uint64_t, Vector<BatchFromMesh>, std::greater<uint32_t>> _latestBatches;
			RenderBatch::Pointer _bboxBatch;
		};
	}
}
