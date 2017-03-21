/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>
#include <et/scene3d/drawer/drawer.h>

namespace et
{
namespace s3d
{

class Drawflow : public Shared
{
public:
	ET_DECLARE_POINTER(Drawflow);

	struct Options
	{
		bool debugDraw = false;
	} options;

public:
	virtual void resizeRenderTargets(const vec2i&) = 0;
	virtual void render() = 0;

	void setDrawer(const Drawer::Pointer& drawer) 
		{ _drawer = drawer;  }

protected:
	Drawer::Pointer& drawer() { return _drawer; }
	const Drawer::Pointer& drawer() const { return _drawer; }

private:
	Drawer::Pointer _drawer;
};

class HDRFlow : public Drawflow
{
public:
	ET_DECLARE_POINTER(HDRFlow);

public:
	HDRFlow(const RenderInterface::Pointer&);

	void resizeRenderTargets(const vec2i&) override;
	void render() override;

private:
	void downsampleLuminance();

private:
	RenderInterface::Pointer _renderer;
	Texture::Pointer _hdrTarget;
	Texture::Pointer _luminanceTarget;

	struct Batches
	{
		Material::Pointer debugMaterial;
		RenderBatch::Pointer debug;
		RenderBatch::Pointer final;

		RenderPassBeginInfo downsampleBeginInfo;
		Material::Pointer downsampleMaterial;
		RenderBatch::Pointer downsample;

		Material::Pointer resolveMaterial;
	} _batches;
	
	RenderPass::Pointer _downsamplePass;
	RenderPass::Pointer _finalPass;
};

}
}
