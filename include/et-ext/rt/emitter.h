/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/imaging/texturedescription.h>
#include <et-ext/rt/raytraceobjects.h>
#include <et-ext/rt/image.h>

namespace et
{
namespace rt
{

class Scene;
class Emitter : public Shared
{
public:
	ET_DECLARE_POINTER(Emitter);
	using Collection = Vector<Pointer>;

public:
	virtual ~Emitter() = default;
	virtual float4 sample(const Scene&, const float4& position, const float4& direction) = 0;
};

class UniformEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(UniformEmitter);

public:
	UniformEmitter(const float4& color);
	float4 sample(const Scene&, const float4& position, const float4& direction) override;

private:
	float4 _color;
};

class EnvironmentEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(EnvironmentEmitter);

public:
	EnvironmentEmitter(const Image::Pointer&);
	float4 sample(const Scene&, const float4& position, const float4& direction) override;

private:
	Image::Pointer _image;
};

class MeshEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(MeshEmitter);

public:
	MeshEmitter(index firstTriangle, index numTriangles, index materialIndex);
	float4 sample(const Scene&, const float4& position, const float4& direction) override;

private:
	index _firstTriangle = InvalidIndex;
	index _numTriangles = 0;
	index _materialIndex = InvalidIndex;
};

}
}
