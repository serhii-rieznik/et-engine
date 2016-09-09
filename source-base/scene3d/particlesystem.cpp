/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/particlesystem.h>

using namespace et;
using namespace et::s3d;

ParticleSystem::ParticleSystem(RenderContext* rc, uint32_t maxSize, const std::string& name, BaseElement* parent)
    : BaseElement(name, parent)
    , _emitter(maxSize)
    , _decl(true, VertexAttributeUsage::Position, DataType::Vec3)
{
	_decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);

	/*
	 * Init geometry
	 */
	VertexStorage::Pointer vs = VertexStorage::Pointer::create(_decl, maxSize);
	IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, maxSize, PrimitiveType::Points);
	auto pos = vs->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	auto clr = vs->accessData<DataType::Vec4>(VertexAttributeUsage::Color, 0);
	for (uint32_t i = 0; i < pos.size(); ++i)
	{
		const auto& p = _emitter.particle(i);
		pos[i] = p.position;
		clr[i] = p.color;
	}
	ia->linearize(maxSize);

	_capacity = vs->capacity();

	_vao = rc->renderer()->createVertexArrayObject(name);
	auto vb = rc->renderer()->createVertexBuffer(name + "-vb", vs, BufferDrawType::Stream);
	auto ib = rc->renderer()->createIndexBuffer(name + "-ib", ia, BufferDrawType::Static);
	_vao->setBuffers(vb, ib);
	
	_timer.expired.connect(this, &ParticleSystem::onTimerUpdated);
	_timer.start(currentTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

ParticleSystem* ParticleSystem::duplicate()
{
	ET_FAIL("Not implemeneted");
	return nullptr;
}

void ParticleSystem::onTimerUpdated(NotifyTimer* timer)
{
	_emitter.update(timer->actualTime());

	_vao->bind();
	
	void* bufferData = _vao->vertexBuffer()->map(0, _capacity,
		MapBufferOptions::Write | MapBufferOptions::InvalidateBuffer);

	auto posOffset = _decl.elementForUsage(VertexAttributeUsage::Position).offset();
	auto clrOffset = _decl.elementForUsage(VertexAttributeUsage::Color).offset();
	
	RawDataAcessor<vec3> pos(reinterpret_cast<char*>(bufferData), _capacity, _decl.totalSize(), posOffset);
	RawDataAcessor<vec4> clr(reinterpret_cast<char*>(bufferData), _capacity + clrOffset, _decl.totalSize(), clrOffset);
	
	for (uint32_t i = 0; i < _emitter.activeParticlesCount(); ++i)
	{
		const auto& p = _emitter.particle(i);
		pos[i] = p.position;
		clr[i] = p.color;
	}
	_vao->vertexBuffer()->unmap();
}
