/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/pipelinestate.h>

namespace et
{

class PipelineStateCachePrivate
{
public:
	Vector<PipelineState::Pointer> cache;
};

PipelineStateCache::PipelineStateCache()
{
	ET_PIMPL_INIT(PipelineStateCache)
}

PipelineStateCache::~PipelineStateCache()
{
	ET_PIMPL_FINALIZE(PipelineStateCache)
}

PipelineState::Pointer PipelineStateCache::find(const VertexDeclaration& decl, Program::Pointer program,
	const DepthState& ds, const BlendState& bs, CullMode cm, TextureFormat tf, PrimitiveType pt)
{
	for (const PipelineState::Pointer& ps : _private->cache)
	{
		if (ps->inputLayout() != decl) continue;
		if (ps->program() != program) continue;
		if (ps->depthState() != ds) continue;
		if (ps->blendState() != bs) continue;
		if (ps->cullMode() != cm) continue;
		if (ps->renderTargetFormat() != tf) continue;
		if (ps->primitiveType() != pt) continue;

		return ps;
	}

	return PipelineState::Pointer();
}

void PipelineStateCache::addToCache(PipelineState::Pointer ps)
{
	PipelineState::Pointer existingState = find(ps->inputLayout(), ps->program(), ps->depthState(),
		ps->blendState(), ps->cullMode(), ps->renderTargetFormat(), ps->primitiveType());

	ET_ASSERT(existingState.invalid());

	if (existingState.invalid())
	{
		_private->cache.push_back(ps);
	}
}

void PipelineStateCache::clear()
{
	_private->cache.clear();
}

}
