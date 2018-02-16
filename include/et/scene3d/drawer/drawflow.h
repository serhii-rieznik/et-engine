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
		bool drawLuminance = false;
		bool drawVelocity = false;
		bool drawShadows = false;
		bool drawAO = false;
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
	HDRFlow(RenderInterface::Pointer&);

	void setColorGradingTable(const std::string&);
	void resizeRenderTargets(const vec2i&) override;
	void render() override;

private:
	void motionBlur();
	void downsampleLuminance();
	void tonemap();
	void antialias();
	void debugDraw();

	enum : uint32_t
	{
		MotionBlurPassCount = 2
	};

	using PostprocessStep = Texture::Pointer(HDRFlow::*)(Texture::Pointer);

	Texture::Pointer luminanceStep(Texture::Pointer);
	Texture::Pointer tonemapStep(Texture::Pointer);
	Texture::Pointer anitialiasStep(Texture::Pointer);

	void addStep(PostprocessStep);
	Texture::Pointer executeSteps();

private:
	RenderInterface::Pointer _renderer;

	Vector<PostprocessStep> _steps;

	Texture::Pointer _primaryTarget;

	Texture::Pointer _colorGradingTexture;
	Sampler::Pointer _colorGradingSampler;

	struct LuminanceStep
	{
		RenderPass::Pointer pass;
		RenderBatch::Pointer batch;
		Compute::Pointer downsample;
		Texture::Pointer downsampled;
		Compute::Pointer adaptation;
		Texture::Pointer computed;
	} _lum;

	struct TAAStep
	{
		RenderPass::Pointer pass;
		RenderBatch::Pointer batch;
		Texture::Pointer history;
	} _taa;

	struct TonemapStep
	{
		RenderPass::Pointer pass;
		RenderBatch::Pointer batch;
	} _tonemap;

	struct Materials
	{
		Material::Pointer debug;
		Material::Pointer posteffects;
	} _materials;
	
	struct Passes
	{
		RenderPass::Pointer final;
	} _passes;

	bool _enableMotionBlur = false;
};

}
}
