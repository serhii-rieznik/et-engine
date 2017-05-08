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

	printVariables("Material variables", _reflection.materialVariables, MaterialVariable_max, mtlToStr);
	printVariables("Object variables", _reflection.objectVariables, ObjectVariable_max, objToStr);

	for (const auto& tex : _reflection.textures)
	{
		if (!tex.second.textures.empty())
		{
			log::info("Vertex textures: { ");
			for (const auto& pv : tex.second.textures)
				log::info("\t%s : %u", pv.first.c_str(), pv.second);
			log::info("}");
		}

		if (!tex.second.samplers.empty())
		{
			log::info("Vertex samplers: { ");
			for (const auto& pv : tex.second.samplers)
				log::info("\t%s : %u", pv.first.c_str(), pv.second);
			log::info("}");
		}
	}
}

void Program::Reflection::serialize(std::ostream& file) const
{
	uint32_t e = inputLayout.numElements();
	serializeUInt32(file, e);
	for (uint32_t i = 0; i < e; ++i)
	{
		serializeUInt32(file, static_cast<uint32_t>(inputLayout[i].usage()));
		serializeUInt32(file, static_cast<uint32_t>(inputLayout[i].type()));
	}

	serializeUInt32(file, objectVariablesBufferSize);
	serializeUInt32(file, ObjectVariable_max);
	for (uint32_t i = 0; i < ObjectVariable_max; ++i)
	{
		serializeUInt32(file, objectVariables[i].offset);
		serializeUInt32(file, objectVariables[i].sizeInBytes);
		serializeUInt32(file, objectVariables[i].arraySize);
		serializeUInt32(file, objectVariables[i].enabled);
	}

	serializeUInt32(file, materialVariablesBufferSize);
	serializeUInt32(file, MaterialVariable_max);
	for (uint32_t i = 0; i < MaterialVariable_max; ++i)
	{
		serializeUInt32(file, materialVariables[i].offset);
		serializeUInt32(file, materialVariables[i].sizeInBytes);
		serializeUInt32(file, materialVariables[i].arraySize);
		serializeUInt32(file, materialVariables[i].enabled);
	}

	auto serializeMap = [&file](const UnorderedMap<std::string, uint32_t>& m) {
		serializeUInt32(file, static_cast<uint32_t>(m.size()));
		for (const auto& i : m)
		{
			serializeString(file, i.first);
			serializeUInt32(file, i.second);
		}
	};
	/*
	 * TODO : serialize
	 *
	serializeMap(textures.vertexTextures);
	serializeMap(textures.vertexSamplers);
	serializeMap(textures.fragmentTextures);
	serializeMap(textures.fragmentSamplers);
	// */
}

bool Program::Reflection::deserialize(std::istream& file)
{
	uint32_t e = deserializeUInt32(file);
	for (uint32_t i = 0; i < e; ++i)
	{
		VertexAttributeUsage usage = static_cast<VertexAttributeUsage>(deserializeUInt32(file));
		DataType type = static_cast<DataType>(deserializeUInt32(file));
		inputLayout.push_back(usage, type);
	}

	objectVariablesBufferSize = deserializeUInt32(file);
	uint32_t varCount = deserializeUInt32(file);
	for (uint32_t i = 0; i < varCount; ++i)
	{
		objectVariables[i].offset = deserializeUInt32(file);
		objectVariables[i].sizeInBytes = deserializeUInt32(file);
		objectVariables[i].arraySize = deserializeUInt32(file);
		objectVariables[i].enabled = deserializeUInt32(file);
	}

	materialVariablesBufferSize = deserializeUInt32(file);
	varCount = deserializeUInt32(file);
	for (uint32_t i = 0; i < varCount; ++i)
	{
		materialVariables[i].offset      = deserializeUInt32(file);
		materialVariables[i].sizeInBytes = deserializeUInt32(file);
		materialVariables[i].arraySize   = deserializeUInt32(file);
		materialVariables[i].enabled     = deserializeUInt32(file);
	}

	auto deserializeMap = [&file](UnorderedMap<std::string, uint32_t>& m) {
		uint32_t size = deserializeUInt32(file);
		for (uint32_t i = 0; i < size; ++i)
		{
			std::string k = deserializeString(file);
			uint32_t v = deserializeUInt32(file);
			m.emplace(k, v);
		}
	};
	
	/*
	 * TODO : deserialize
	 *
	deserializeMap(textures.vertexTextures);
	deserializeMap(textures.vertexSamplers);
	deserializeMap(textures.fragmentTextures);
	deserializeMap(textures.fragmentSamplers);
	// */

	return true;
}


}
