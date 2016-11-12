/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/pipelinestate.h>

namespace et
{

void PipelineState::buildBuffers()
{
	objectVariablesBuffer.resize(reflection.objectVariablesBufferSize);
	objectVariablesBuffer.fill(0);
}

void PipelineState::uploadObjectVariable(const String& name, const void* ptr, uint32_t size)
{
	auto var = reflection.objectVariables.find(name);
	if (var != reflection.objectVariables.end())
	{
		auto* dst = objectVariablesBuffer.element_ptr(var->second.offset);
		memcpy(dst, ptr, size);
	}
}

void PipelineState::printReflection()
{
	std::map<uint32_t, String> sortedFields;
	auto printVariables = [&sortedFields](const char* tag, const PipelineState::VariableMap& input) {
		if (input.empty()) return;

		sortedFields.clear();
		for (const auto& pv : input)
			sortedFields.emplace(pv.second.offset, pv.first);

		log::info("%s: { ", tag);
		for (const auto& pv : sortedFields)
			log::info("\t%s : %u", pv.second.c_str(), pv.first);
		log::info("}");
	};

	printVariables("Pass variables", reflection.passVariables);
	printVariables("Material variables", reflection.materialVariables);
	printVariables("Object variables", reflection.objectVariables);
	
	if (!reflection.vertexTextures.empty())
	{
		log::info("Vertex textures: { ");
		for (const auto& pv : reflection.vertexTextures)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!reflection.vertexSamplers.empty())
	{
		log::info("Vertex samplers: { ");
		for (const auto& pv : reflection.vertexSamplers)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!reflection.fragmentTextures.empty())
	{
		log::info("Fragment textures: { ");
		for (const auto& pv : reflection.fragmentTextures)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!reflection.fragmentSamplers.empty())
	{
		log::info("Fragment samplers: { ");
		for (const auto& pv : reflection.fragmentSamplers)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}
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
