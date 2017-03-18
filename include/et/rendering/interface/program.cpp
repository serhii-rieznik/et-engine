/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/program.h>

namespace et
{

inline const std::string& objToStr(uint32_t i) { return objectVariableToString(static_cast<ObjectVariable>(i)); }
inline const std::string& mtlToStr(uint32_t i) { return materialVariableToString(static_cast<MaterialVariable>(i)); }
inline const std::string& glbToStr(uint32_t i) { return globalVariableToString(static_cast<GlobalVariable>(i)); }

void Program::printReflection() const
{
	using conv = const std::string&(uint32_t);

	std::map<uint32_t, std::string> sortedFields;
	auto printVariables = [&sortedFields](const char* tag, const Variable* input, uint32_t count, conv cnv) {
		sortedFields.clear();

		for (uint32_t i = 0; i < count; ++i)
			sortedFields.emplace(input[i].offset, cnv(i));

		log::info("%s: { ", tag);
		for (const auto& pv : sortedFields)
			log::info("\t%s : %u", pv.second.c_str(), pv.first);
		log::info("}");
	};

	printVariables("Global variables", _reflection.globalVariables, GlobalVariable_max, glbToStr);
	printVariables("Material variables", _reflection.materialVariables, MaterialVariable_max, mtlToStr);
	printVariables("Object variables", _reflection.objectVariables, ObjectVariable_max, objToStr);
	
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
