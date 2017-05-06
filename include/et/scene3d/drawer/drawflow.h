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
	virtual ~Drawflow() { } 
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
	void postprocess();
	void downsampleLuminance();
	void tonemap();
	void antialias();
	void debugDraw();

	enum : uint32_t
	{
		MotionBlurPassCount = 2
	};

private:
	RenderInterface::Pointer _renderer;
	Texture::Pointer _primaryTarget;
	Texture::Pointer _secondaryTarget;
	Texture::Pointer _luminanceTarget;
	Texture::Pointer _luminanceHistory;
	Texture::Pointer _renderHistory;

	struct Materials
	{
		Material::Pointer debug;
		Material::Pointer posteffects;
	} _materials;

	struct Batches
	{
		RenderPassBeginInfo downsampleBeginInfo;
		RenderBatch::Pointer debug;
		RenderBatch::Pointer final;
		RenderBatch::Pointer tonemap;
		RenderBatch::Pointer downsample;
		RenderBatch::Pointer motionBlur;
		RenderBatch::Pointer txaa;
	} _batches;
	
	struct Passes
	{
		RenderPass::Pointer motionBlur0;
		RenderPass::Pointer motionBlur1;
		RenderPass::Pointer downsample;
		RenderPass::Pointer tonemapping;
		RenderPass::Pointer txaa;
		RenderPass::Pointer final;
	} _passes;
};

}
}
