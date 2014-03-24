/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/particlesystem.h>

using namespace et;
using namespace et::s3d;

ParticleSystem::ParticleSystem(RenderContext* rc, size_t maxSize, const std::string& name, Element* parent) :
	Element(name, parent), _rc(rc), _decl(true, Usage_Position, Type_Vec3)
{
	_decl.push_back(Usage_Color, Type_Vec4);
	
	/*
	 * Init particles vector
	 */
	_particles.resize(maxSize);

	/*
	 * Init geometry
	 */
	VertexArray::Pointer va = VertexArray::Pointer::create(_decl, maxSize);
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat_16bit, maxSize, PrimitiveType_Points);
	auto pos = va->chunk(Usage_Position).accessData<vec3>(0);
	auto clr = va->chunk(Usage_Color).accessData<vec4>(0);
	for (size_t i = 0; i < pos.size(); ++i)
	{
		const auto& p = _particles.at(i);
		pos[i] = p.position;
		clr[i] = p.color;
	}
	ia->linearize(va->size());
	
	_vao = rc->vertexBufferFactory().createVertexArrayObject(name, va, BufferDrawType_Stream, ia, BufferDrawType_Static);
	_vertexData = va->generateDescription();
	
	_updateFunction = [](Particle& p, float t, float dt)
	{
		p.velocity += dt * p.acceleration;
		p.position += dt * p.velocity;
	};
	
	_timer.expired.connect(this, &ParticleSystem::onTimerUpdated);
	_updateTime = mainTimerPool()->actualTime();
	_timer.start(mainTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

ParticleSystem* ParticleSystem::duplicate()
{
	ET_FAIL("Not implemeneted");
	return nullptr;
}

void ParticleSystem::onTimerUpdated(NotifyTimer* timer)
{
	float currentTime = timer->actualTime();
	float deltaTime = currentTime - _updateTime;
	
	_updateTime = currentTime;
	
	void* bufferData = nullptr;
	
	_rc->renderState().bindVertexArray(_vao);

	_vao->vertexBuffer()->map(&bufferData, 0, _vertexData.data.dataSize(),
		VertexBufferData::MapBufferMode_WriteOnly);
	
	auto posOffset = _decl.elementForUsage(Usage_Position).offset();
	auto clrOffset = _decl.elementForUsage(Usage_Color).offset();
	RawDataAcessor<vec3> pos(reinterpret_cast<char*>(bufferData), _vertexData.data.dataSize(), _decl.dataSize(), posOffset);
	RawDataAcessor<vec4> clr(reinterpret_cast<char*>(bufferData), _vertexData.data.dataSize() + clrOffset, _decl.dataSize(), clrOffset);
	
	_activeParticlesCount = 0;
	
	for (auto& p : _particles)
	{
		_updateFunction(p, currentTime, deltaTime);
		
		if (p.color.w > 0.0)
		{
			pos[_activeParticlesCount] = p.position;
			clr[_activeParticlesCount] = p.color;
			++_activeParticlesCount;
		}
	}
	
	log::info("%zu particles", _activeParticlesCount);
	
	_vao->vertexBuffer()->unmap();
}

bool ParticleSystem::emitParticle(const vec3& origin, const vec3& vel, const vec3& accel, const vec4& color, float lifeTime)
{
	float currentTime = _timer.actualTime();
	
	for (auto& p : _particles)
	{
		if (currentTime - p.emitTime > p.lifeTime)
		{
			p.position = origin;
			p.velocity = vel;
			p.acceleration = accel;
			p.color = color;
			p.emitTime = currentTime;
			p.lifeTime = lifeTime;
			return true;
		}
	}
	
	return false;
}
