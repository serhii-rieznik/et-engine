/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/systems/cubemapprocessor.h>

namespace et
{

void CubemapProcessor::wrapEquirectangularTextureToCubemap(RenderInterface::Pointer renderer, const Texture::Pointer& input, const Texture::Pointer& output)
{
	ET_ASSERT(input->target() == TextureTarget::Texture_2D);
	ET_ASSERT(output->target() == TextureTarget::Texture_Cube);

	RenderPass::ConstructionInfo passInfo;
	passInfo.camera = _camera;
	passInfo.color[0].enabled = true;
	passInfo.color[0].texture = output;
	passInfo.color[0].loadOperation = FramebufferOperation::Clear;
	passInfo.color[0].storeOperation = FramebufferOperation::Store;
	passInfo.color[0].useDefaultRenderTarget = false;
	passInfo.name = "eq-to-cubemap";

	Material::Pointer material = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));

	RenderPass::Pointer pass = renderer->allocateRenderPass(passInfo);
	pass->begin();
	pass->pushRenderBatch(renderhelper::createFullscreenRenderBatch(input, material));
	pass->end();
	
	renderer->submitRenderPass(pass);
}

}