/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		struct Particle
		{
			vec3 position;
			vec3 velocity;
			vec3 acceleration;
			vec4 color;
			
			float emitTime = 0.0f;
			float lifeTime = 0.0f;
		};
		
		class ParticleSystem : public Element, public EventReceiver
		{
		public:
			ET_DECLARE_POINTER(ParticleSystem)
						
		public:
			ParticleSystem(RenderContext*, size_t, const std::string&, Element*);
			
			ElementType type() const
				{ return ElementType_ParticleSystem; }
			
			ParticleSystem* duplicate();
			
			const VertexArrayObject& vao() const
				{ return _vao; }

			const IndexBuffer& indexBuffer() const
				{ return _vao->indexBuffer(); }
			
			size_t activeParticlesCount() const
				{ return _activeParticlesCount; }
			
			bool emitParticle(const vec3& origin, const vec3& vel, const vec3& accel,
				const vec4& color, float lifeTime);
			
			size_t emitParticles(const vec3* origins, const vec3* velocities, size_t count, const vec3& accel,
				const vec4& color, float lifeTime, float lifeTimeVariation);
			
			template <typename F>
			void setUpdateFunction(const F& func)
				{ _updateFunction = func; }
			
		private:
			void onTimerUpdated(NotifyTimer*);
						
		private:
			RenderContext* _rc = nullptr;
			
			VertexArrayObject _vao;
			VertexDeclaration _decl;
			VertexArray::Description _vertexData;
			
			NotifyTimer _timer;
			
			std::vector<Particle> _particles;
			std::function<void(Particle&, float, float)> _updateFunction;
			
			float _updateTime = 0.0f;
			size_t _activeParticlesCount = 0;
		};
	}
}
