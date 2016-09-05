/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/helpers.h>

namespace et
{
namespace renderhelper
{

namespace rh_local
{
	Material::Pointer default2DMaterial;
	VertexArrayObject::Pointer default2DPlane;
}

void init(RenderContext* rc)
{
	ET_ASSERT(rh_local::default2DPlane.invalid());
	ET_ASSERT(rh_local::default2DMaterial.invalid());

	auto vd = VertexDeclaration(false, VertexAttributeUsage::Position, DataType::Vec2);
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 4, PrimitiveType::TriangleStrips);
	VertexStorage::Pointer vs = VertexStorage::Pointer::create(vd, 4);
	auto pos = vs->accessData<DataType::Vec2>(VertexAttributeUsage::Position, 0);
	pos[0] = vec2(-1.0f, -1.0f);
	pos[1] = vec2( 1.0f, -1.0f);
	pos[2] = vec2(-1.0f,  1.0f);
	pos[3] = vec2( 1.0f,  1.0f);
	ia->linearize(4);

	rh_local::default2DPlane = rc->renderer()->createVertexArrayObject("rh_local::mesh");
	auto vb = rc->renderer()->createVertexBuffer("rh_local::vb", vs, BufferDrawType::Static);
	auto ib = rc->renderer()->createIndexBuffer("rh_local::ib", ia, BufferDrawType::Static);
	rh_local::default2DPlane->setBuffers(vb, ib);

	auto materialFile = application().resolveFileName("engine_data/materials/2d.material");
	ET_ASSERT(fileExists(materialFile));

    rh_local::default2DMaterial = Material::Pointer::create(rc->renderer().ptr());
	rh_local::default2DMaterial->loadFromJson(loadTextFile(materialFile), getFilePath(materialFile));
}

void release()
{
	rh_local::default2DPlane.reset(nullptr);
	rh_local::default2DMaterial.reset(nullptr);
}
	
RenderBatch::Pointer createFullscreenRenderBatch(Texture::Pointer texture)
{
	ET_ASSERT(rh_local::default2DMaterial.valid());
	ET_ASSERT(rh_local::default2DPlane.valid());
	rh_local::default2DMaterial->setTexutre("color_texture", texture);
	return RenderBatch::Pointer::create(rh_local::default2DMaterial, rh_local::default2DPlane);
}

}
}
