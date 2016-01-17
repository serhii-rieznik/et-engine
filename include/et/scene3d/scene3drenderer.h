/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>

namespace et
{
	namespace s3d
	{		
		class Renderer : public FlagsHolder
		{
		public:
			ET_DECLARE_POINTER(Renderer)
			
			enum : uint64_t
			{
				RenderMeshes = 0x01,
				RenderHelperMeshes = 0x02,
				Wireframe = 0x04,
				
				RenderAll = RenderMeshes | RenderHelperMeshes
			};
			
		public:
			Renderer();
			void render(RenderContext*, const Scene&);
			
			void initDebugObjects(RenderContext*, Material::Pointer bboxMaterial);
			
		private:
			void renderMeshList(RenderContext*, const s3d::BaseElement::List&);
			void renderTransformedBoundingBox(RenderContext*, const BoundingBox&, const mat4&);
			
		private:
			using BatchFromMesh = std::pair<et::RenderBatch::Pointer, et::s3d::Mesh::Pointer>;
			std::map<uint32_t, std::vector<BatchFromMesh>, std::greater<uint32_t>> _latestBatches;
			RenderBatch::Pointer _bboxBatch;
		};
	}
}
