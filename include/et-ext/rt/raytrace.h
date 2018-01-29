/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/kdtree.h>
#include <et-ext/rt/integrator.h>
#include <et-ext/rt/rtscene.h>

namespace et {
namespace rt {

class RaytracePrivate;

class ET_ALIGNED(16) Raytrace {
	ET_DECLARE_PIMPL(Raytrace, 4096);

public:
	using OutputMethod = std::function<void(const vec2i& /* location */, const vec4& /* color */ )>;

public:
	Raytrace();
	~Raytrace();

	template <typename F>
	void setOutputMethod(F func) {
		_outputMethod = func;
	}

	void setIntegrator(EvaluateFunction);

	void output(const vec2i&, const vec4&);

	void perform(s3d::Scene::Pointer, const vec2i&);
	vec4 performAtPoint(const vec2i&);

	void stop();
	void setOptions(const Options&);

	void renderSpacePartitioning();
	void waitForCompletion();

	bool running() const;
	void reportProgress();

	ET_DECLARE_EVENT0(renderFinished);

private:
	friend class RaytracePrivate;
	OutputMethod _outputMethod;
};

}
}
