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

#define ET_VULKAN_PROGRAM_USE_CACHE 0

const uint32_t programCacheVersion = 1;
const uint32_t programCacheHeader = 'PROG';
const uint32_t programCacheVertex = 'VERT';
const uint32_t programCacheFragment = 'FRAG';
const uint32_t programCacheReflection = 'REFL';

namespace et
{

class VulkanProgramPrivate : public VulkanShaderModules
{
public:
	VulkanProgramPrivate(VulkanState& v)
		: vulkan(v) { }

	VulkanState& vulkan;

	bool loadCached(uint64_t hash, std::vector<uint32_t>& vert, std::vector<uint32_t>& frag, Program::Reflection& reflection);
	void saveCached(uint64_t hash, const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag, const Program::Reflection& reflection);
	void build(const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag);

	std::string cacheFolder();
};

VulkanProgram::VulkanProgram(VulkanState& v)
{
	ET_PIMPL_INIT(VulkanProgram, v);
}

VulkanProgram::~VulkanProgram()
{
	if (_private->vertex)
		vkDestroyShaderModule(_private->vulkan.device, _private->vertex, nullptr);

	if (_private->fragment)
		vkDestroyShaderModule(_private->vulkan.device, _private->fragment, nullptr);

	ET_PIMPL_FINALIZE(VulkanProgram);
}

void VulkanProgram::build(const std::string& source)
{
	std::vector<uint32_t> vertexBin;
	std::vector<uint32_t> fragmentBin;
	
	std::hash<std::string> sourceHash;
	uint64_t hsh = sourceHash(source);

	bool canBuild = false;
	
	if (_private->loadCached(hsh, vertexBin, fragmentBin, _reflection))
	{
		canBuild = true;
	}
	else 
	{
		vertexBin.clear();
		fragmentBin.clear();
		canBuild = hlslToSPIRV(source, vertexBin, fragmentBin, _reflection);
		if (canBuild)
		{
			_private->saveCached(hsh, vertexBin, fragmentBin, _reflection);
		}
	}
	
	if (canBuild)
	{
		_private->build(vertexBin, fragmentBin);
	}
}

const VulkanShaderModules& VulkanProgram::shaderModules() const
{
	return *(_private);
}

bool VulkanProgramPrivate::loadCached(uint64_t hash, std::vector<uint32_t>& vert, std::vector<uint32_t>& frag,
	Program::Reflection& reflection)
{
#if ET_VULKAN_PROGRAM_USE_CACHE
	char buffer[1024] = {};
	std::string baseFolder = cacheFolder();
	sprintf(buffer, "%s%016llX.program.cache", baseFolder.c_str(), hash);
	std::string cacheFile(buffer);

	if (!fileExists(cacheFile))
		return false;

	std::ifstream file(cacheFile, std::ios::in | std::ios::binary);
	uint32_t header = deserializeUInt32(file);
	if (header != programCacheHeader)
		return false;
	
	uint32_t version = deserializeUInt32(file);
	if (version > programCacheVersion)
		return false;

	uint32_t vertexId = deserializeUInt32(file);
	if (vertexId != programCacheVertex)
		return false;

	uint32_t vertexSize = deserializeUInt32(file);
	vert.resize(vertexSize);
	file.read(reinterpret_cast<char*>(vert.data()), vertexSize * sizeof(uint32_t));

	uint32_t fragmentId = deserializeUInt32(file);
	if (fragmentId != programCacheFragment)
		return false;

	uint32_t fragmentSize = deserializeUInt32(file);
	frag.resize(fragmentSize);
	file.read(reinterpret_cast<char*>(frag.data()), fragmentSize * sizeof(uint32_t));

	uint32_t reflectionId = deserializeUInt32(file);
	if (reflectionId != programCacheReflection)
		return false;

	reflection.deserialize(file);
	return true;

#else
	return false;
#endif
}

void VulkanProgramPrivate::saveCached(uint64_t hash, const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag, 
	const Program::Reflection& reflection)
{
#if (ET_VULKAN_PROGRAM_USE_CACHE)
	char buffer[1024] = {};
	std::string baseFolder = cacheFolder();
	sprintf(buffer, "%s%016llX.program.cache", baseFolder.c_str(), hash);
	std::string cacheFile(buffer);

	std::ofstream file(cacheFile, std::ios::out | std::ios::binary);
	serializeUInt32(file, programCacheHeader);
	serializeUInt32(file, programCacheVersion);
	
	serializeUInt32(file, programCacheVertex);
	serializeUInt32(file, static_cast<uint32_t>(vert.size()));
	file.write(reinterpret_cast<const char*>(vert.data()), vert.size() * sizeof(uint32_t));

	serializeUInt32(file, programCacheFragment);
	serializeUInt32(file, static_cast<uint32_t>(frag.size()));
	file.write(reinterpret_cast<const char*>(frag.data()), frag.size() * sizeof(uint32_t));

	serializeUInt32(file, programCacheReflection);
	reflection.serialize(file);
#endif
}

void VulkanProgramPrivate::build(const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag)
{
	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

	createInfo.pCode = vert.data();
	createInfo.codeSize = vert.size() * sizeof(uint32_t);
	VULKAN_CALL(vkCreateShaderModule(vulkan.device, &createInfo, nullptr, &vertex));

	createInfo.pCode = frag.data();
	createInfo.codeSize = frag.size() * sizeof(uint32_t);
	VULKAN_CALL(vkCreateShaderModule(vulkan.device, &createInfo, nullptr, &fragment));

	stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stageCreateInfo[0].module = vertex;
	stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stageCreateInfo[1].module = fragment;

	stageCreateInfo[0].pName = "vertexMain";
	stageCreateInfo[1].pName = "fragmentMain";
}

std::string VulkanProgramPrivate::cacheFolder()
{
	std::string result = application().environment().applicationDocumentsFolder() + "shadercache/";
	
	if (!fileExists(result))
		createDirectory(result, true);

	return result;
}

}
