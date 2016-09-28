/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/scene3drenderer.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/primitives.h>

using namespace et;
using namespace et::s3d;

s3d::Renderer::Renderer() :
	FlagsHolder(RenderAll)
{
	
}

void s3d::Renderer::render(RenderContext* rc, const Scene& scene, const Camera& camera)
{
    if (hasFlag(RenderMeshes) == false)
        return;

    auto lights = scene.childrenOfType(et::s3d::ElementType::Light);
    auto lightPosition = camera.position();
    if (lights.empty() == false)
    {
        auto light = static_cast<s3d::Light::Pointer>(lights.front());
        lightPosition = light->camera().position();
    }
    
	RenderPass::ConstructionInfo passInfo;
	passInfo.target.colorLoadOperation = et::FramebufferOperation::Clear;
	passInfo.target.depthLoadOperation = et::FramebufferOperation::Clear;
	passInfo.target.clearColor = vec4(0.25f, 0.3333f, 0.5f, 1.0f);
	passInfo.camera = camera;
	passInfo.defaultLightPosition = lightPosition;

	RenderPass::Pointer pass = rc->renderer()->allocateRenderPass(passInfo);
	renderMeshList(pass, scene.childrenOfType(s3d::ElementType::Mesh));
	rc->renderer()->submitRenderPass(pass);
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

	auto cameraPosition = pass->info().camera.position();
	for (auto& rbv : _latestBatches)
	{
		std::sort(rbv.second.begin(), rbv.second.end(), [cameraPosition](BatchFromMesh& l, BatchFromMesh& r)
		{
			auto lip = l.first->transformation() * l.first->boundingBox().center;
			auto rip = r.first->transformation() * r.first->boundingBox().center;
			if (l.first->material()->blendState().enabled)
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
}

void s3d::Renderer::initDebugObjects(RenderContext* rc, Material::Pointer bboxMaterial)
{
	/*
	 * TODO
	 *
	VertexDeclaration decl(false, et::VertexAttributeUsage::Position, et::DataType::Vec3);
	VertexArray::Pointer va = VertexArray::Pointer::create(decl, 0);
	primitives::createBox(va, vec3(1.0f));
	
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, va->size(), PrimitiveType::Triangles);
	ia->linearize(ia->capacity());
	
	auto cube = rc->renderer()->createVertexArrayObject("cube-vao");
	auto cubeVB = rc->renderer()->createVertexBuffer("cube-vb", va, BufferDrawType::Static);
	auto cubeIB = rc->renderer()->createIndexBuffer("cube-ib", ia, BufferDrawType::Static);
	cube->setBuffers(cubeVB, cubeIB);

	_bboxBatch = RenderBatch::Pointer::create(bboxMaterial, cube);
	*/
}

void s3d::Renderer::renderTransformedBoundingBox(RenderPass::Pointer pass, const BoundingBox& b, const mat4& t)
{
    if (hasFlag(RenderDebugObjects))
    {
        _bboxBatch->material()->setProperty("bboxScale", b.halfDimension);
        _bboxBatch->material()->setProperty("bboxCenter", b.center);
        _bboxBatch->setTransformation(t);
        pass->pushRenderBatch(_bboxBatch);
    }
}
