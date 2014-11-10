/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/particlesystem.h>

using namespace et;
using namespace et::s3d;

ParticleSystem::ParticleSystem(RenderContext* rc, size_t maxSize, const std::string& name, Element* parent) :
	Element(name, parent), _rc(rc), _decl(true, Usage_Position, Type_Vec3), _emitter(maxSize)
{
	_decl.push_back(Usage_Color, Type_Vec4);

	/*
	 * Init geometry
	 */
	VertexArray::Pointer va = VertexArray::Pointer::create(_decl, maxSize);
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat_16bit, maxSize, PrimitiveType_Points);
	auto pos = va->chunk(Usage_Position).accessData<vec3>(0);
	auto clr = va->chunk(Usage_Color).accessData<vec4>(0);
	for (size_t i = 0; i < pos.size(); ++i)
	{
		const auto& p = _emitter.particle(i);
		pos[i] = p.position;
		clr[i] = p.color;
	}
	ia->linearize(va->size());
	
	_vao = rc->vertexBufferFactory().createVertexArrayObject(name, va, BufferDrawType_Stream, ia, BufferDrawType_Static);
	_vertexData = va->generateDescription();
	
	_timer.expired.connect(this, &ParticleSystem::onTimerUpdated);
	_timer.start(mainTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

ParticleSystem* ParticleSystem::duplicate()
{
	ET_FAIL("Not implemeneted");
	return nullptr;
}

void ParticleSystem::onTimerUpdated(NotifyTimer* timer)
{
	_emitter.update(timer->actualTime());
	
	void* bufferData = nullptr;

	auto posOffset = _decl.elementForUsage(Usage_Position).offset();
	auto clrOffset = _decl.elementForUsage(Usage_Color).offset();
	RawDataAcessor<vec3> pos(reinterpret_cast<char*>(bufferData), _vertexData.data.dataSize(), _decl.dataSize(), posOffset);
	RawDataAcessor<vec4> clr(reinterpret_cast<char*>(bufferData), _vertexData.data.dataSize() + clrOffset, _decl.dataSize(), clrOffset);
	
	_rc->renderState().bindVertexArray(_vao);
	
	bufferData = _vao->vertexBuffer()->map(0, _vertexData.data.dataSize(), VertexBufferData::MapBufferMode_WriteOnly);
	for (size_t i = 0; i < _emitter.activeParticlesCount(); ++i)
	{
		const auto& p = _emitter.particle(i);
		pos[i] = p.position;
		clr[i] = p.color;
	}
	_vao->vertexBuffer()->unmap();
}

bool ParticleSystem::emitParticle(const vec3& origin, const vec3& vel, const vec3& accel, const vec4& color, float lifeTime)
{
	_emitter.base().position = origin;
	_emitter.base().velocity = vel;
	_emitter.base().acceleration = accel;
	_emitter.base().color = color;
	_emitter.base().lifeTime = lifeTime;
	
	return _emitter.emit(1, mainTimerPool()->actualTime()) == 1;
}

size_t ParticleSystem::emitParticles(const vec3* o, const vec3* v, size_t amount, const vec3& accel, const vec4& clr, float lt)
{
	size_t emitted = 0;
	for (; emitted < amount; ++emitted)
	{
		if (!emitParticle(o[emitted], v[emitted], accel, clr, lt))
			break;
	}
	return emitted;
}

