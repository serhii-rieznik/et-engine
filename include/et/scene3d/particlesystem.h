/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/baseelement.h>
#include <et-ext/helpers/particles.h>

namespace et
{
namespace s3d
{
class ParticleSystem : public BaseElement, public EventReceiver
{
public:
	ET_DECLARE_POINTER(ParticleSystem);

public:
	ParticleSystem(RenderContext*, uint32_t capacity, const std::string&, BaseElement*);

	ElementType type() const
	{
		return ElementType::ParticleSystem;
	}

	ParticleSystem* duplicate();

	const VertexStream::Pointer& vertexStream() const
	{
		return _vertexStream;
	}

	const Buffer::Pointer& indexBuffer() const
	{
		return _vertexStream->indexBuffer();
	}

	size_t activeParticlesCount() const
	{
		return _emitter.activeParticlesCount();
	}

	particles::PointSpriteEmitter& emitter()
	{
		return _emitter;
	}

	const particles::PointSpriteEmitter& emitter() const
	{
		return _emitter;
	}

	template <typename F>
	void setUpdateFunction(const F& func)
	{
		_emitter.setUpdateFunction(func);
	}

private:
	void onTimerUpdated(NotifyTimer*);

private:
	particles::PointSpriteEmitter _emitter;
	VertexStream::Pointer _vertexStream;
	VertexDeclaration _decl;
	NotifyTimer _timer;
	uint32_t _capacity = 0;
};
}
}
