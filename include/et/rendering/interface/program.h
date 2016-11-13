/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>
#include <et/rendering/base/vertexdeclaration.h>
#include <et/rendering/interface/textureset.h>

namespace et
{
class Program : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Program);

	struct Variable
	{
		uint32_t offset = 0;
		uint32_t size = 0;
	};
	using VariableMap = UnorderedMap<String, Variable>;

	struct Reflection
	{
		VertexDeclaration inputLayout;

		VariableMap passVariables;
		uint32_t passVariablesBufferSize = 0;

		VariableMap materialVariables;
		uint32_t materialVariablesBufferSize = 0;

		VariableMap objectVariables;
		uint32_t objectVariablesBufferSize = 0;

		TextureSet::Reflection textures;
	};

public:
	virtual void build(const std::string& source) = 0;

	const Reflection& reflection() const
		{ return _reflection; }

	void printReflection() const;

protected:
	Reflection _reflection;
};

inline void Program::printReflection() const
{
	std::map<uint32_t, String> sortedFields;
	auto printVariables = [&sortedFields](const char* tag, const Program::VariableMap& input) {
		if (input.empty()) return;

		sortedFields.clear();
		for (const auto& pv : input)
			sortedFields.emplace(pv.second.offset, pv.first);

		log::info("%s: { ", tag);
		for (const auto& pv : sortedFields)
			log::info("\t%s : %u", pv.second.c_str(), pv.first);
		log::info("}");
	};

	printVariables("Pass variables", _reflection.passVariables);
	printVariables("Material variables", _reflection.materialVariables);
	printVariables("Object variables", _reflection.objectVariables);
	
	if (!_reflection.textures.vertexTextures.empty())
	{
		log::info("Vertex textures: { ");
		for (const auto& pv : _reflection.textures.vertexTextures)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!_reflection.textures.vertexSamplers.empty())
	{
		log::info("Vertex samplers: { ");
		for (const auto& pv : _reflection.textures.vertexSamplers)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!_reflection.textures.fragmentTextures.empty())
	{
		log::info("Fragment textures: { ");
		for (const auto& pv : _reflection.textures.fragmentTextures)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}

	if (!_reflection.textures.fragmentSamplers.empty())
	{
		log::info("Fragment samplers: { ");
		for (const auto& pv : _reflection.textures.fragmentSamplers)
			log::info("\t%s : %u", pv.first.c_str(), pv.second);
		log::info("}");
	}
}

}
