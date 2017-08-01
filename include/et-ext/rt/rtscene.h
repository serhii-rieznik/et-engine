/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/raytraceobjects.h>
#include <et-ext/rt/kdtree.h>
#include <et-ext/rt/bsdf.h>
#include <et-ext/rt/emitter.h>
#include <et-ext/rt/sampler.h>
#include <et/rendering/base/renderbatch.h>
#include <et/rendering/objects/light.h>

namespace et
{
namespace rt
{

class ET_ALIGNED(16) Scene
{
public:
	struct SceneEntry
	{
		RenderBatch::Pointer batch;
		Light::Pointer light;
		mat4 transformation;

		SceneEntry(const Light::Pointer& l)
			: light(l) { }

		SceneEntry(const RenderBatch::Pointer& rb, const mat4& t)
			: batch(rb), transformation(t) { }
	};

public:
	void build(const Vector<SceneEntry>&, const Camera::Pointer&);
	void addEmitter(const Emitter::Pointer&);

public:
	Options options;

	KDTree kdTree;
	Material::Collection materials;
	Emitter::Collection emitters;
	HammersleyQMCSampler sampler;
	
	float focalDistance = 0.0f;
	ray3d centerRay;
};

}
}
