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
#include <fstream>

#define ET_VULKAN_PROGRAM_USE_CACHE 0

namespace et
{

class VulkanProgramPrivate : public VulkanShaderModules
{
public:
	VulkanProgramPrivate(VulkanState& v)
		: vulkan(v)
	{
	}

	VulkanState& vulkan;

	bool loadCached(uint64_t hash, std::vector<uint32_t>& vert, std::vector<uint32_t>& frag);
	void saveCached(uint64_t hash, const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag);
	void build(const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag);
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
	bool canBuild = false;
	std::vector<uint32_t> vertexBin;
	std::vector<uint32_t> fragmentBin;
	std::hash<std::string> sourceHash;
	uint64_t hsh = sourceHash(source);

	bool cacheLoaded = _private->loadCached(hsh, vertexBin, fragmentBin);
	if (cacheLoaded || hlslToSPIRV(source, vertexBin, fragmentBin, _reflection))
	{
		_private->saveCached(hsh, vertexBin, fragmentBin);
		_private->build(vertexBin, fragmentBin);
	}
}

const VulkanShaderModules& VulkanProgram::shaderModules() const
{
	return *(_private);
}

bool VulkanProgramPrivate::loadCached(uint64_t hash, std::vector<uint32_t>& vert, std::vector<uint32_t>& frag)
{
#if ET_VULKAN_PROGRAM_USE_CACHE
	const std::string& baseFolder = application().environment().applicationDocumentsFolder();
	std::string cacheFile;
	{
		char buffer[1024] = {};
		sprintf(buffer, "%s%016llX.vert", baseFolder.c_str(), hash);
		vertFile = buffer;
	}
	std::string fragFile;
	{
		char buffer[1024] = {};
		sprintf(buffer, "%s%016llX.frag", baseFolder.c_str(), hash);
		fragFile = buffer;
	}

	if (!fileExists(vertFile) || !fileExists(fragFile))
		return false;

	std::ifstream vertIn(vertFile, std::ios::in | std::ios::binary);
	{
		uint32_t vertSize = streamSize(vertIn);
		vert.resize(vertSize / sizeof(uint32_t));
		vertIn.read(reinterpret_cast<char*>(vert.data()), vertSize);
	}
	
	std::ifstream fragIn(fragFile, std::ios::in | std::ios::binary);
	{
		uint32_t fragSize = streamSize(fragIn);
		frag.resize(fragSize / sizeof(uint32_t));
		fragIn.read(reinterpret_cast<char*>(frag.data()), fragSize);
	}
	return !(vertIn.fail() && fragIn.fail());
#else
	return false;
#endif
}

void VulkanProgramPrivate::saveCached(uint64_t hash, const std::vector<uint32_t>& vert, const std::vector<uint32_t>& frag)
{
#if (ET_VULKAN_PROGRAM_USE_CACHE)
	const std::string& baseFolder = application().environment().applicationDocumentsFolder();
	std::string vertFile;
	{
		char buffer[1024] = {};
		sprintf(buffer, "%s%016llX.vert", baseFolder.c_str(), hash);
		vertFile = buffer;
	}
	std::string fragFile;
	{
		char buffer[1024] = {};
		sprintf(buffer, "%s%016llX.frag", baseFolder.c_str(), hash);
		fragFile = buffer;
	}

	std::ofstream vertOut(vertFile, std::ios::out | std::ios::binary);
	vertOut.write(reinterpret_cast<const char*>(vert.data()), vert.size() * sizeof(uint32_t));

	std::ofstream fragOut(fragFile, std::ios::out | std::ios::binary);
	fragOut.write(reinterpret_cast<const char*>(frag.data()), frag.size() * sizeof(uint32_t));
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

}
