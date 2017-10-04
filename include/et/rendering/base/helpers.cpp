/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/interface/renderer.h>
#include <et/rendering/base/helpers.h>

namespace et
{
namespace renderhelper
{

namespace rh_local
{
RenderContext* renderContext = nullptr;
Material::Pointer texturedMaterial;
VertexStream::Pointer default2DPlane;
}

void init(RenderContext* rc)
{
	ET_ASSERT(rh_local::default2DPlane.invalid());

	rh_local::renderContext = rc;
	
	VertexDeclaration vd(false, VertexAttributeUsage::Position, DataType::Vec3);
	VertexStorage::Pointer vs = VertexStorage::Pointer::create(vd, 3);

	VertexDataAccessor<DataType::Vec3> pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	pos[0] = vec3(-1.0f, -1.0f, 0.0f);
	pos[1] = vec3( 3.0f, -1.0f, 0.0f);
	pos[2] = vec3(-1.0f,  3.0f, 0.0f);

	Buffer::Pointer vb = rc->renderer()->createVertexBuffer("rh_local::vb", vs, Buffer::Location::Device);
	rh_local::default2DPlane = VertexStream::Pointer::create();
	rh_local::default2DPlane->setVertexBuffer(vb, vs->declaration());

	rh_local::texturedMaterial = rc->renderer()->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::Textured2D);

	Sampler::Description defaultSampler;
	rh_local::texturedMaterial->setSampler(MaterialTexture::BaseColor, rc->renderer()->createSampler(defaultSampler));
}

void release()
{
	rh_local::default2DPlane.reset(nullptr);
	rh_local::texturedMaterial.reset(nullptr);
}
	
RenderBatch::Pointer createFullscreenRenderBatch(const Texture::Pointer& texture, Material::Pointer mat)
{
	ET_ASSERT(rh_local::default2DPlane.valid());
	ET_ASSERT(rh_local::texturedMaterial.valid());

	if (mat.invalid())
		mat = rh_local::texturedMaterial;

	RenderBatch::Pointer batch = rh_local::renderContext->renderer()->allocateRenderBatch();
	{
		MaterialInstance::Pointer materialInstance = mat->instance();
		materialInstance->setTexture(MaterialTexture::BaseColor, texture);
		batch->construct(materialInstance, rh_local::default2DPlane, 0, 3);
	}
	return batch;
}

RenderBatch::Pointer createFullscreenRenderBatch(const Texture::Pointer& tex, const Material::Pointer& mat, const Sampler::Pointer& smp)
{
	RenderBatch::Pointer rb = createFullscreenRenderBatch(tex, mat);
	rb->material()->setSampler(MaterialTexture::BaseColor, smp);
	return rb;
}

RenderBatch::Pointer createFullscreenRenderBatch(const Texture::Pointer& tex, const Material::Pointer& mat, const Sampler::Pointer& smp, const ResourceRange& rng)
{
	RenderBatch::Pointer rb = createFullscreenRenderBatch(tex, mat, smp);
	rb->material()->setTexture(MaterialTexture::BaseColor, tex, rng);
	return rb;
}


}
}
