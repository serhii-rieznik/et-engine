/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/rtscene.h>

namespace et
{
namespace rt
{

// dummy, to be removed
struct EnvironmentSampler : public Shared { ET_DECLARE_POINTER(EnvironmentSampler); };

class Integrator : public Object
{
public:
	ET_DECLARE_POINTER(Integrator);

public:
	virtual ~Integrator() {}

	virtual float4 evaluate(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength) 
	{
		return float4(0.18f);
	}

	virtual float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength,
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&)
	{
		return float4(0.18f);
	}
};

class NormalsIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(NormalsIntegrator);

public:
	float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength, 
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&) override;
};

class FresnelIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(FresnelIntegrator);

public:
	float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength, 
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&) override;
};

class AmbientOcclusionIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(AmbientOcclusionIntegrator);

public:
	float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength, 
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&) override;
};

class AmbientOcclusionHackIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(AmbientOcclusionHackIntegrator);

public:
	float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength, 
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&) override;
};

class PathTraceIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(PathTraceIntegrator);

public:
	float4 gather(const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength, 
		KDTree&, EnvironmentSampler::Pointer&, const Material::Collection&) override;
};

struct BackwardPathTracingIntegrator : public Integrator
{
public:
	ET_DECLARE_POINTER(BackwardPathTracingIntegrator);

public:
	float4 evaluate(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength) override;
};

}
}
