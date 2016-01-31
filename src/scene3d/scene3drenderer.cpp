/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/scene3drenderer.h>
#include <et/rendering/primitives.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/rendersystem.h>

using namespace et;
using namespace et::s3d;

s3d::Renderer::Renderer() :
	FlagsHolder(RenderAll)
{
	
}

void s3d::Renderer::render(RenderContext* rc, const Scene& scene, const Camera& camera)
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
	
	RenderSystem renderSystem(rc);
	auto pass = renderSystem.allocateRenderPass({camera});
	renderMeshList(pass, allMeshes);
	renderSystem.submitRenderPass(pass);
	
	rs.setFillMode(FillMode::Solid);
}

void s3d::Renderer::renderMeshList(RenderPass::Pointer pass, const s3d::BaseElement::List& meshes)
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

	auto cameraPosition = pass->camera().position();
	for (auto& rbv : _latestBatches)
	{
		std::sort(rbv.second.begin(), rbv.second.end(), [cameraPosition](BatchFromMesh& l, BatchFromMesh& r)
		{
			auto lip = l.first->transformation() * l.first->boundingBox().center;
			auto rip = r.first->transformation() * r.first->boundingBox().center;
			if (l.first->material()->blendState().blendEnabled)
				return (lip - cameraPosition).dotSelf() > (rip - cameraPosition).dotSelf();
			else
				return (lip - cameraPosition).dotSelf() < (rip - cameraPosition).dotSelf();
		});

		for (auto& rb : rbv.second)
		{
			rb.second->material()->bindToMaterial(rb.first->material());
			pass->pushRenderBatch(rb.first);
		}
	}
	
	if (hasFlag(RenderDebugObjects))
	{
		for (auto& rbv : _latestBatches)
		{
			for (auto& rb : rbv.second)
			{
				renderTransformedBoundingBox(pass, rb.first->boundingBox(), rb.first->transformation());
			}
		}
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

void s3d::Renderer::renderTransformedBoundingBox(RenderPass::Pointer pass, const BoundingBox& b, const mat4& t)
{
	_bboxBatch->material()->setProperty("bboxScale", b.halfDimension);
	_bboxBatch->material()->setProperty("bboxCenter", b.center);
	_bboxBatch->setTransformation(t);
	pass->pushRenderBatch(_bboxBatch);
}
