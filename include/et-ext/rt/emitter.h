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

struct ET_ALIGNED(16) EmitterInteraction
{
	float4 direction;
	float4 normal;
	float4 color = float4(0.0f);
	float cosTheta = 0.0f;
	float pdf = 0.0f;
};

class Scene;
class Emitter : public Object
{
public:
	ET_DECLARE_POINTER(Emitter);
	using Collection = Vector<Pointer>;

	enum class Type : uint32_t
	{
		Uniform,
		Environment,
		Area,
	};

public:
	Emitter(Type t) : 
		_type(t) { }

	virtual void prepare(const Scene&)
		{ }

	virtual float4 samplePoint(const Scene&) const
		{ return float4(0.0f); }

	virtual float4 evaluate(const Scene&, const float4& position, const float4& direction, float4& nrm, float4& pos, float& pdf) const
		{ return float4(0.0f); }

	virtual float pdf(const float4& position, const float4& direction, const float4& lightPosition, const float4& lightNormal) const
		{ return 0.0f; }

	virtual bool containsTriangle(uint32_t) const { return false; }

	virtual uint32_t materialIndex() const
		{ return InvalidIndex; }

	Type type() const { return _type; }

private:
	Type _type = Type::Uniform;
};

class UniformEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(UniformEmitter);

public:
	UniformEmitter(const float4& color);
	float4 samplePoint(const Scene&) const override;
	float4 evaluate(const Scene&, const float4& position, const float4& direction, float4& nrm, float4& pos, float& pdf) const override;
	float pdf(const float4& position, const float4& direction, const float4& lightPosition, const float4& lightNormal) const override;

private:
	float4 _color;
};

class EnvironmentEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(EnvironmentEmitter);

public:
	EnvironmentEmitter(const Image::Pointer&);

private:
	Image::Pointer _image;
};

class MeshEmitter : public Emitter
{
public:
	ET_DECLARE_POINTER(MeshEmitter);

public:
	MeshEmitter(uint32_t firstTriangle, uint32_t numTriangles, uint32_t materialIndex);
	void prepare(const Scene&) override;

	uint32_t materialIndex() const override { return _materialIndex; }

	float4 samplePoint(const Scene&) const override;
	float4 evaluate(const Scene&, const float4& position, const float4& direction, float4& nrm, float4& pos, float& pdf) const override;
	float pdf(const float4& position, const float4& direction, const float4& lightPosition, const float4& lightNormal) const override;

	bool containsTriangle(uint32_t t) const override {
		return (t >= _firstTriangle) && (t < _firstTriangle + _numTriangles);
	};

private:
	uint32_t _firstTriangle = InvalidIndex;
	uint32_t _numTriangles = 0;
	uint32_t _materialIndex = InvalidIndex;
	float _area = 0.0f;
};

}
}
