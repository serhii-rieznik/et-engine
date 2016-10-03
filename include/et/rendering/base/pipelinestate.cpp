/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/pipelinestate.h>

namespace et
{

void PipelineState::printReflection()
{
	log::info("Pass variables: { ");
	for (const auto& pv : reflection.passVariables)
	{
		log::info("\t%s : %u", pv.first.c_str(), pv.second.offset);
	}
	log::info("}");

	log::info("Material variables: { ");
	for (const auto& pv : reflection.materialVariables)
	{
		log::info("\t%s : %u", pv.first.c_str(), pv.second.offset);
	}
	log::info("}");

	log::info("Object variables: { ");
	for (const auto& pv : reflection.objectVariables)
	{
		log::info("\t%s : %u", pv.first.c_str(), pv.second.offset);
	}
	log::info("}");

	log::info("Vertex textures: { ");
	for (const auto& pv : reflection.vertexTextures)
	{
		log::info("\t%s : %u", pv.first.c_str(), pv.second);
	}
	log::info("}");

	log::info("Fragment textures: { ");
	for (const auto& pv : reflection.fragmentTextures)
	{
		log::info("\t%s : %u", pv.first.c_str(), pv.second);
	}
	log::info("}");
}

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

PipelineState::Pointer PipelineStateCache::find(const VertexDeclaration& decl,
	VertexStream::Pointer vs, Program::Pointer program, const DepthState& ds,
	const BlendState& bs, CullMode cm, TextureFormat tf)
{
	for (const PipelineState::Pointer& ps : _private->cache)
	{
		if (ps->inputLayout() != decl) continue;
		if (ps->vertexStream() != vs) continue;
		if (ps->program() != program) continue;
		if (ps->depthState() != ds) continue;
		if (ps->blendState() != bs) continue;
		if (ps->cullMode() != cm) continue;
		if (ps->renderTargetFormat() != tf) continue;
		return ps;
	}

	return PipelineState::Pointer();
}

void PipelineStateCache::addToCache(PipelineState::Pointer ps)
{
	ET_ASSERT(find(ps->inputLayout(), ps->vertexStream(), ps->program(),
		ps->depthState(), ps->blendState(), ps->cullMode(), ps->renderTargetFormat()).invalid());

	_private->cache.push_back(ps);
}

}
