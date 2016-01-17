/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/scene3drenderer.h>
#include <et/rendering/primitives.h>
#include <et/rendering/rendercontext.h>

using namespace et;
using namespace et::s3d;

s3d::Renderer::Renderer() :
	FlagsHolder(RenderAll)
{
	
}

void s3d::Renderer::render(RenderContext* rc, const Scene& scene)
{
	auto& rs = rc->renderState();
	
	bool renderNormal = hasFlag(RenderMeshes);
	bool renderHelper = hasFlag(RenderHelperMeshes);
	
	s3d::BaseElement::List allMeshes = scene.childrenOfType(s3d::ElementType::Mesh);
	allMeshes.erase(std::remove_if(allMeshes.begin(), allMeshes.end(),
		[renderNormal, renderHelper](s3d::Mesh::Pointer m)
	{
		bool isHelper = m->hasFlag(s3d::Flag_Helper);
		return (isHelper && !renderHelper) || (!isHelper && !renderNormal);
	}), allMeshes.end());
	
	rs.setFillMode(hasFlag(Wireframe) ? FillMode::Wireframe : FillMode::Solid);
	renderMeshList(rc, allMeshes);
	rs.setFillMode(FillMode::Solid);
}

void s3d::Renderer::renderMeshList(RenderContext* rc, const s3d::BaseElement::List& meshes)
{
	for (auto& lb : _latestBatches)
	{
		lb.second.clear();
	}
	
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->prepareRenderBatches();
		for (auto& rb : mesh->renderBatches())
		{
			_latestBatches[rb->material()->sortingKey()].emplace_back(rb, mesh);
		}
	}

/*
	auto cam = rc->renderer()->currentCamera();
	for (auto& rbv : _latestBatches)
	{
		auto& v = rbv.second;
		std::sort(v.begin(), v.end(), [cam](BatchFromMesh& l, BatchFromMesh& r)
		{
			// TODO
		});
	}
*/
	
	static uint32_t frameIndex = 0;
	const uint32_t frameStep = 1;
	const uint32_t renderGroup = 1;
	
	uint32_t i = 0;
	for (auto& rbv : _latestBatches)
	{
		if (i == renderGroup)
		{
			uint32_t j = 0;
			
			for (auto& rb : rbv.second)
			{
				if (j < frameIndex)
				{
					rb.second->material()->bindToMaterial(rb.first->material());
					rc->renderer()->pushRenderBatch(rb.first);
					
					renderTransformedBoundingBox(rc, rb.first->boundingBox(), rb.second->finalTransform());
				}
				++j;
			}
			
			frameIndex += frameStep;
			if (frameIndex >= rbv.second.size())
			{
				frameIndex = 0;
			}
			break;
		}
		++i;
	}
}

void s3d::Renderer::initDebugObjects(RenderContext* rc, Material::Pointer bboxMaterial)
{
	VertexDeclaration decl(false, et::VertexAttributeUsage::Position, et::DataType::Vec3);
	VertexArray::Pointer va = VertexArray::Pointer::create(decl, 0);
	primitives::createBox(va, vec3(1.0f));
	
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, va->size(), PrimitiveType::Triangles);
	ia->linearize(ia->capacity());
	
	auto cube = rc->vertexBufferFactory().createVertexArrayObject("cube-vao", va, BufferDrawType::Static, ia, BufferDrawType::Static);
	_bboxBatch = RenderBatch::Pointer::create(bboxMaterial, cube);
}

void s3d::Renderer::renderTransformedBoundingBox(RenderContext* rc, const BoundingBox& b, const mat4& t)
{
	_bboxBatch->setTransformation(t);
	rc->renderer()->pushRenderBatch(_bboxBatch);
}
