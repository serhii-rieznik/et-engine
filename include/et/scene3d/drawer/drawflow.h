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

class Drawflow : public Object
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

	void setColorGradingTable(const std::string&);
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
	Texture::Pointer _luminanceHistogram;
	Texture::Pointer _renderHistory;
	Texture::Pointer _colorGradingTexture;
	Sampler::Pointer _colorGradingSampler;

	struct Materials
	{
		Material::Pointer debug;
		Material::Pointer posteffects;
		Material::Pointer computeTest;
	} _materials;
	
	struct Passes
	{
		RenderPass::Pointer logLuminance;
		RenderPassBeginInfo logLuminanceBeginInfo;
		
		RenderPass::Pointer averageLuminance;
		RenderPassBeginInfo averageLuminanceBeginInfo;
		
		RenderPass::Pointer resolveLuminance;
		RenderPassBeginInfo resolveLuminanceBeginInfo;

		RenderPass::Pointer motionBlur0;
		RenderPass::Pointer motionBlur1;
		RenderPass::Pointer tonemapping;
		RenderPass::Pointer txaa;
		RenderPass::Pointer final;
	} _passes;
};

}
}
