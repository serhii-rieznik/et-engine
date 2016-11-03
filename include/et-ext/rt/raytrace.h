/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/kdtree.h>
#include <et-ext/rt/integrator.h>
#include <et-ext/rt/environment.h>

namespace et {

class RaytracePrivate;
class Raytrace
{
public:
	using OutputMethod = std::function<void(const vec2i&, const vec4&)>;

	enum class Method : uint32_t
	{
		PathTracing,
		LightTracing
	};
	
	struct Options
	{
		uint32_t threads = 0;
		uint32_t raysPerPixel = 32;
		uint32_t maxPathLength = 0;
		uint32_t maxKDTreeDepth = 0;
		uint32_t renderRegionSize = 32;
		float apertureSize = 0.0f;
		float focalDistanceCorrection = 0.0f;
		Method method = Method::PathTracing;
		bool renderKDTree = false;
	};

public:
	Raytrace();
	~Raytrace();

	template <typename F>
	void setOutputMethod(F func)
		{ _outputMethod = func; }
	
	void setIntegrator(rt::Integrator::Pointer);
	void setEnvironmentSampler(rt::EnvironmentSampler::Pointer);
	
	void output(const vec2i&, const vec4&);

	void perform(s3d::Scene::Pointer, const Camera&, const vec2i&);
	vec4 performAtPoint(s3d::Scene::Pointer, const Camera&, const vec2i&, const vec2i&);

	void stop();
	void setOptions(const Options&);
	
	void renderSpacePartitioning();
	
	ET_DECLARE_EVENT0(renderFinished)

private:
	friend class RaytracePrivate;
	ET_DECLARE_PIMPL(Raytrace, 2048);
	OutputMethod _outputMethod;
};

}
