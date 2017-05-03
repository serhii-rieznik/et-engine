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
	Material::Pointer texturedMaterial;
	VertexStream::Pointer default2DPlane;
}

void init(RenderContext* rc)
{
	ET_ASSERT(rh_local::default2DPlane.invalid());

	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 4, PrimitiveType::TriangleStrips);

	auto vd = VertexDeclaration(false, VertexAttributeUsage::Position, DataType::Vec3);
	vd.push_back(VertexAttributeUsage::TexCoord0, et::DataType::Vec2);
	VertexStorage::Pointer vs = VertexStorage::Pointer::create(vd, 4);

	auto pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	auto tc0 = vs->accessData<DataType::Vec2>(VertexAttributeUsage::TexCoord0, 0);
	pos[0] = vec3(-1.0f, -1.0f, 0.0f); tc0[0] = vec2(0.0f, 0.0f);
	pos[1] = vec3( 1.0f, -1.0f, 0.0f); tc0[1] = vec2(1.0f, 0.0f);
	pos[2] = vec3(-1.0f,  1.0f, 0.0f); tc0[2] = vec2(0.0f, 1.0f);
	pos[3] = vec3( 1.0f,  1.0f, 0.0f); tc0[3] = vec2(1.0f, 1.0f);
	ia->linearize(4);

	auto vb = rc->renderer()->createVertexBuffer("rh_local::vb", vs, Buffer::Location::Device);
	auto ib = rc->renderer()->createIndexBuffer("rh_local::ib", ia, Buffer::Location::Device);
	
	rh_local::default2DPlane = VertexStream::Pointer::create();
	rh_local::default2DPlane->setVertexBuffer(vb, vs->declaration());
	rh_local::default2DPlane->setIndexBuffer(ib, ia->format(), ia->primitiveType());

	rh_local::texturedMaterial = rc->renderer()->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::Textured2D);

	Sampler::Description defaultSampler;
	rh_local::texturedMaterial->setSampler(MaterialTexture::BaseColor, rc->renderer()->createSampler(defaultSampler));
}

void release()
{
	rh_local::default2DPlane.reset(nullptr);
	rh_local::texturedMaterial.reset(nullptr);
}
	
RenderBatch::Pointer renderhelper::createFullscreenRenderBatch(const Texture::Pointer& texture, Material::Pointer mat)
{
	ET_ASSERT(rh_local::default2DPlane.valid());
	ET_ASSERT(rh_local::texturedMaterial.valid());

	if (mat.invalid())
		mat = rh_local::texturedMaterial;

	MaterialInstance::Pointer materialInstance = mat->instance();
	materialInstance->setTexture(MaterialTexture::BaseColor, texture);
	
	return RenderBatch::Pointer::create(materialInstance, rh_local::default2DPlane, 0, rh_local::default2DPlane->vertexCount());
}

}
}
