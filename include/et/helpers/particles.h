//
//  particles.h
//  Prime Elements
//
//  Created by Sergey Reznik on 3/11/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#pragma once

#include <et/core/et.h>

namespace et
{
	namespace particles
	{
		struct PointSprite
		{
			vec3 position;
			vec3 velocity;
			vec3 acceleration;
			vec4 color = vec4(1.0f);
			float size = 1.0f;
			float emitTime = 0.0f;
			float lifeTime = 0.0f;
		};
		
		template <typename P>
		inline void defaultMovementFunction(P& p, float t, float dt)
		{
			p.color.w = clamp(1.0f - (t - p.emitTime) / p.lifeTime, 0.0f, 1.0f);
			
			p.velocity += dt * p.acceleration;
			p.position += dt * p.velocity;
		}
		
		template <typename P>
		inline P defaultVariationFunction(const P& base, const P& var, float emitTime)
		{
			PointSprite e = base;
			e.position += randVector(var.position.x, var.position.y, var.position.z);
			e.velocity += randVector(var.velocity.x, var.velocity.y, var.velocity.z);
			e.acceleration += randVector(var.acceleration.x, var.acceleration.y, var.acceleration.z);
			e.color += vec4(randVector(var.color.x, var.color.y, var.color.z), randomFloat(-var.color.w, var.color.w));
			e.size += randomFloat(-var.size, var.size);
			e.emitTime = emitTime + randomFloat(-var.emitTime, var.emitTime);
			e.lifeTime += randomFloat(-var.lifeTime, var.lifeTime);
			return e;
		}
		
		template <typename P>
		class Emitter
		{
		public:
			ET_DECLARE_POINTER(Emitter)

			typedef std::vector<P, SharedBlockAllocatorSTDProxy<P>> ParticleList;
			typedef std::function<void(P&, float, float)> ParticleUpdateFuction;
			typedef std::function<P(const P&, const P&, float)> ParticleVariationFuction;
			
		public:
			Emitter(size_t capacity) :
				_particles(capacity) { }
			
			template <typename F>
			void setUpdateFunction(F&& func)
				{ _updateFunction = func; }

			template <typename V>
			void setVariationFunction(V&& func)
				{ _variationFunction = func; }
			
			const P& particle(size_t i)
			{
				ET_ASSERT(i < _particles.size());
				return _particles.at(i);
			}
			
			size_t activeParticlesCount() const
				{ return _activeParticles; }
			
			bool emit(const P& p)
			{
				if (_activeParticles >= _particles.size()) return false;
				
				_particles.at(_activeParticles++) = p;
				return true;
			}
			
			size_t emit(size_t count, const P& base, const P& var)
			{
				_lastBase = base;
				_lastVariation = var;
				
				size_t emitted = 0;
				while (emitted < count)
				{
					P e = _variationFunction(_lastBase, _lastVariation, base.emitTime);
					if (emit(e))
					{
						_updateFunction(e, base.emitTime, 0.0f);
						++emitted;
					}
					else
					{
						break;
					}
				}
				return emitted;
			}
			
			void update(float t)
			{
				if (_updateTime == 0.0f)
					_updateTime = t;
				
				float dt = t - _updateTime;
				_updateTime = t;
				
				for (size_t i = 0; i < _activeParticles; ++i)
				{
					auto& particle = _particles.at(i);
					
					_updateFunction(particle, t, dt);
					
					if ((t - particle.emitTime) > particle.lifeTime)
					{
						if (_autoRenewParticles)
						{
							particle = _variationFunction(_lastBase, _lastVariation, t);
							_updateFunction(particle, t, 0.0f);
						}
						else
						{
							std::swap(particle, _particles.at(_activeParticles - 1));
							_activeParticles--;
						}
					}
				}
			}
			
		private:
			ParticleList _particles;
			ParticleUpdateFuction _updateFunction = defaultMovementFunction<P>;
			ParticleVariationFuction _variationFunction = defaultVariationFunction<P>;
			
			P _lastBase;
			P _lastVariation;
			
			size_t _activeParticles = 0;
			float _updateTime = 0.0f;
			bool _autoRenewParticles = true;
		};
		
		typedef Emitter<PointSprite> PointSpriteEmitter;
		
	};
}
