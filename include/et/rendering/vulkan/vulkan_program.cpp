/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/shadersource.h>
#include <et/rendering/vulkan/glslang/vulkan_glslang.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan.h>
#include <et/app/application.h>
#include <et/core/tools.h>
#include <et/core/serialization.h>
#include <fstream>

#define ET_VULKAN_PROGRAM_USE_CACHE 1

const uint32_t programCacheVersion = 2;
const uint32_t programCacheHeader = 'PROG';
const uint32_t programCacheVertex = 'VERT';
const uint32_t programCacheFragment = 'FRAG';
const uint32_t programCacheCompute = 'COMP';
const uint32_t programCacheReflection = 'REFL';

namespace et
{

class VulkanProgramPrivate : public VulkanShaderModules
{
public:
	VulkanProgramPrivate(VulkanState& v)
		: vulkan(v) { }

	VulkanState& vulkan;

	bool loadCached(uint32_t stages, uint64_t hash, SPIRProgramStageMap& dst, Program::Reflection& ref);
	void saveCached(uint32_t stages, uint64_t hash, const SPIRProgramStageMap& src, const Program::Reflection& ref);
	void addProgramStage(ProgramStage stage, const std::vector<uint32_t>& code);

	std::string cacheFolder();
	std::string cacheFile(uint32_t stages, uint64_t hash);
};

VulkanProgram::VulkanProgram(VulkanState& v)
{
	ET_PIMPL_INIT(VulkanProgram, v);
}

VulkanProgram::~VulkanProgram()
{
	for (const auto& module : _private->stageCreateInfos)
		vkDestroyShaderModule(_private->vulkan.device, module.second.module, nullptr);

	ET_PIMPL_FINALIZE(VulkanProgram);
}

void VulkanProgram::build(uint32_t stages, const std::string& source)
{
	SPIRProgramStageMap requestedStages;

	if (stages & static_cast<uint32_t>(ProgramStage::Vertex)) 
		requestedStages.emplace(ProgramStage::Vertex, SPIRSource());

	if (stages & static_cast<uint32_t>(ProgramStage::Fragment)) 
		requestedStages.emplace(ProgramStage::Fragment, SPIRSource());

	if (stages & static_cast<uint32_t>(ProgramStage::Compute)) 
		requestedStages.emplace(ProgramStage::Compute, SPIRSource());

	uint64_t hash = std::hash<std::string>().operator()(source);

	if (_private->loadCached(stages, hash, requestedStages, _reflection) == false)
	{
		generateSPIRFromHLSL(source, requestedStages, _reflection);
		_private->saveCached(stages, hash, requestedStages, _reflection);
	}

	for (const auto& stage : requestedStages)
	{
		if (stage.second.size() > 0)
			_private->addProgramStage(stage.first, stage.second);
	}
}

const VulkanShaderModules& VulkanProgram::shaderModules() const
{
	return *(_private);
}

bool VulkanProgramPrivate::loadCached(uint32_t stages, uint64_t hash, SPIRProgramStageMap& dst, Program::Reflection& ref)
{
#if ET_VULKAN_PROGRAM_USE_CACHE
	std::string fileName = cacheFile(stages, hash);

	if (!fileExists(fileName))
		return false;

	#define RETURN_WITH_ERROR(err) do { log::error(err); return false; } while (0)

	std::ifstream file(fileName, std::ios::in | std::ios::binary);
	uint32_t header = deserializeUInt32(file);
	if (header != programCacheHeader)
		RETURN_WITH_ERROR("Invalid header read from cache file");
	
	uint32_t version = deserializeUInt32(file);
	if (version > programCacheVersion)
		RETURN_WITH_ERROR("Unsupported version read from cache file");

	uint32_t storedStages = deserializeUInt32(file);
	if ((storedStages & stages) != stages)
		RETURN_WITH_ERROR("Stored stages in program cache does not match requested stages");

	uint32_t loadedStages = 0;
	while (file.eof() == false)
	{
		uint32_t chunk = deserializeUInt32(file);
		switch (chunk)
		{
		case programCacheVertex:
		{
			SPIRSource& source = dst.at(ProgramStage::Vertex);
			source.resize(deserializeUInt64(file));
			file.read(reinterpret_cast<char*>(source.data()), source.size() * sizeof(uint32_t));
			loadedStages |= static_cast<uint32_t>(ProgramStage::Vertex);
			break;
		}
		case programCacheFragment:
		{
			SPIRSource& source = dst.at(ProgramStage::Fragment);
			source.resize(deserializeUInt64(file));
			file.read(reinterpret_cast<char*>(source.data()), source.size() * sizeof(uint32_t));
			loadedStages |= static_cast<uint32_t>(ProgramStage::Fragment);
			break;
		}
		case programCacheCompute:
		{
			SPIRSource& source = dst.at(ProgramStage::Compute);
			source.resize(deserializeUInt64(file));
			file.read(reinterpret_cast<char*>(source.data()), source.size() * sizeof(uint32_t));
			loadedStages |= static_cast<uint32_t>(ProgramStage::Compute);
			break;
		}
		case programCacheReflection:
		{
			ref.deserialize(file);
			break;
		}
		default:
			break;
		}
	}

	return true;
	#undef RETURN_WITH_ERROR

#else
	return false;
#endif
}

void VulkanProgramPrivate::saveCached(uint32_t stages, uint64_t hash, const SPIRProgramStageMap& src, const Program::Reflection& ref)
{
#if (ET_VULKAN_PROGRAM_USE_CACHE)
	std::string fileName = cacheFile(stages, hash);

	std::ofstream file(fileName, std::ios::out | std::ios::binary);
	serializeUInt32(file, programCacheHeader);
	serializeUInt32(file, programCacheVersion);
	serializeUInt32(file, stages);

	auto writeStage = [&src, stages, &file](ProgramStage stage) {
		if ((static_cast<uint32_t>(stage) & stages) && (src.count(stage) > 0))
		{
			switch (stage)
			{
			case ProgramStage::Vertex:
				serializeUInt32(file, programCacheVertex);
				break;
			case ProgramStage::Fragment:
				serializeUInt32(file, programCacheFragment);
				break;
			case ProgramStage::Compute:
				serializeUInt32(file, programCacheCompute);
				break;
			default:
				ET_ASSERT(!"Invalid program stage");
			}
			const SPIRSource& source = src.at(stage);
			serializeUInt64(file, source.size());
			file.write(reinterpret_cast<const char*>(source.data()), source.size() * sizeof(uint32_t));
		}
	};

	writeStage(ProgramStage::Vertex);
	writeStage(ProgramStage::Fragment);
	writeStage(ProgramStage::Compute);

	serializeUInt32(file, programCacheReflection);
	ref.serialize(file);
#endif
}

void VulkanProgramPrivate::addProgramStage(ProgramStage stage, const std::vector<uint32_t>& code)
{
	ET_ASSERT(code.size() > 0);

	stageCreateInfos.emplace(stage, VkPipelineShaderStageCreateInfo());
	
	VkPipelineShaderStageCreateInfo& stageCreateInfo = stageCreateInfos[stage];
	stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfo.stage = vulkan::programStageValue(stage);
	stageCreateInfo.pName = vulkan::programStageEntryName(stage);

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.pCode = code.data();
	createInfo.codeSize = code.size() * sizeof(uint32_t);
	VULKAN_CALL(vkCreateShaderModule(vulkan.device, &createInfo, nullptr, &stageCreateInfo.module));
}

std::string VulkanProgramPrivate::cacheFolder()
{
	std::string result = application().environment().applicationDocumentsFolder() + "shadercache/";
	
	if (!fileExists(result))
		createDirectory(result, true);

	return result;
}

std::string VulkanProgramPrivate::cacheFile(uint32_t stages, uint64_t hash)
{
	char buffer[2048] = { };
	std::string baseFolder = cacheFolder();
	sprintf(buffer, "%s%08X-%016llX.program.cache", baseFolder.c_str(), stages, hash);
	return std::string(buffer);
}

}
