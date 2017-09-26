/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/sampler.h>

namespace et
{
namespace rt
{

RandomSampler::RandomSampler(uint32_t maxSamples) :
	_maxSamples(maxSamples), _samples(0)
{
	ET_ASSERT(_maxSamples > 0);
}

bool RandomSampler::next(vec2& sample)
{
	++_samples;
	sample.x = fastRandomFloat();
	sample.y = fastRandomFloat();
	return _samples <= _maxSamples;
}

float4 RandomSampler::sample(uint32_t, uint32_t)
{
	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

UniformSampler::UniformSampler(uint32_t maxSamples)
{
	float sm = std::sqrt(static_cast<float>(maxSamples));
	_gridSubdivisions = static_cast<uint32_t>(sm + 0.5f);
	_maxSamples = _gridSubdivisions * _gridSubdivisions;
	ET_ASSERT(_maxSamples > 0);

	_cellSize = 1.0f / static_cast<float>(_gridSubdivisions);
}

bool UniformSampler::next(vec2& sample)
{
	++_samples;
	if (_samples <= _maxSamples)
	{
		float row = static_cast<float>((_samples - 1) / _gridSubdivisions);
		float col = static_cast<float>((_samples - 1) % _gridSubdivisions);
		sample.x = (0.5f + col) * _cellSize;
		sample.y = (0.5f + row) * _cellSize;
	}
	return _samples <= _maxSamples;
}

StratifiedSampler::StratifiedSampler(uint32_t maxSamples, float a, float b) :
	UniformSampler(maxSamples), _a(a), _b(b)
{
}

bool StratifiedSampler::next(vec2 & sample)
{
	++_samples;
	if (_samples <= _maxSamples)
	{
		float row = static_cast<float>((_samples - 1) / _gridSubdivisions);
		float col = static_cast<float>((_samples - 1) % _gridSubdivisions);
		sample.x = (col + _a + _b * fastRandomFloat()) * _cellSize;
		sample.y = (row + _a + _b * fastRandomFloat()) * _cellSize;
	}
	return _samples <= _maxSamples;
}

float HammersleyQMCSampler::rinv(uint32_t i)
{
	uint32_t bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return static_cast<float>(static_cast<double>(bits) * 2.3283064365386963e-10);
}

float4 HammersleyQMCSampler::sample(uint32_t i, uint32_t dim)
{
	return float4(static_cast<float>(i % dim) / static_cast<float>(dim - 1), rinv(i), 0.0f, 0.0f);
}

}
}
