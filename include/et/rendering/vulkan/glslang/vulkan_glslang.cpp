/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <external/glslang/glslang/Public/ShaderLang.h>
#include <external/glslang/SPIRV/Logger.h>
#include <external/glslang/SPIRV/GlslangToSpv.h>
#include <et/core/et.h>
#include <functional>

#pragma comment(lib, "glslang-default-resource-limits.lib")
#pragma comment(lib, "glslang.lib")
#pragma comment(lib, "OGLCompiler.lib")
#pragma comment(lib, "OSDependent.lib")
#pragma comment(lib, "SPIRV.lib")
#pragma comment(lib, "SPVRemapper.lib")
#pragma comment(lib, "HLSL.lib")

namespace et
{

const TBuiltInResource defaultBuiltInResource = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,
	/* .limits = */ {
		/* .nonInductiveForLoops = */ 1,
		/* .whileLoops = */ 1,
		/* .doWhileLoops = */ 1,
		/* .generalUniformIndexing = */ 1,
		/* .generalAttributeMatrixVectorIndexing = */ 1,
		/* .generalVaryingIndexing = */ 1,
		/* .generalSamplerIndexing = */ 1,
		/* .generalVariableIndexing = */ 1,
		/* .generalConstantMatrixVectorIndexing = */ 1,
	} };

void buildProgramReflection(glslang::TProgram*, PipelineState::Reflection& reflection);

bool glslToSPIRV(const std::string& vertexSource, const std::string& fragmentSource,
	std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin, PipelineState::Reflection& reflection)
{
	glslang::TProgram* program = nullptr;
	EShMessages messages = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules);
	const char* vertexSourceCStr[] = { vertexSource.c_str() };
	const char* vertexFileNames[] = { "Vertex Shader" };
	const char* fragmentSourceCStr[] = { fragmentSource.c_str() };
	const char* fragmentFileNames[] = { "Fragment Shader" };

	glslang::InitializeProcess();

	glslang::TShader vertexShader(EShLanguage::EShLangVertex);
	vertexShader.setStringsWithLengthsAndNames(vertexSourceCStr, nullptr, vertexFileNames, 1);
	vertexShader.setAutoMapBindings(true);

	glslang::TShader fragmentShader(EShLanguage::EShLangFragment);
	fragmentShader.setStringsWithLengthsAndNames(fragmentSourceCStr, nullptr, fragmentFileNames, 1);
	fragmentShader.setAutoMapBindings(true);

	struct OnExit
	{
		std::function<void()> exitFunction;
		OnExit(const std::function<void()>& e) : exitFunction(e) { }
		~OnExit() { exitFunction(); }
	} onExit([&]()
	{
		etDestroyObject(program);
		glslang::FinalizeProcess();
	});

	if (!vertexShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse vertex shader:\n%s", vertexShader.getInfoLog());
		debug::debugBreak();
		return false;
	}
	
	if (!fragmentShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse fragment shader:\n%s", fragmentShader.getInfoLog());
		debug::debugBreak();
		return false;
	}
	
	program = etCreateObject<glslang::TProgram>();
	program->addShader(&vertexShader);
	program->addShader(&fragmentShader);
	if (!program->link(messages))
	{
		log::error("Failed to link program:\n%s", program->getInfoLog());
		debug::debugBreak();
		return false;
	}
	if (!program->mapIO())
	{
		log::error("Failed to map program's IO:\n%s", program->getInfoLog());
		debug::debugBreak();
		return false;
	}

	if (program->buildReflection() == false) 
	{
		log::error("Failed to build reflection:\n%s", program->getInfoLog());
		debug::debugBreak();
		return false;
	}

	buildProgramReflection(program, reflection);

	glslang::TIntermediate* vertexIntermediate = program->getIntermediate(EShLanguage::EShLangVertex);
	if (vertexIntermediate == nullptr)
	{
		log::error("Failed to get vertex binary:\n%s", program->getInfoLog());
		debug::debugBreak();
		return false;
	}
	
	glslang::TIntermediate* fragmentIntermediate = program->getIntermediate(EShLanguage::EShLangFragment);
	if (fragmentIntermediate == nullptr)
	{
		log::error("Failed to get fragment binary:\n%s", program->getInfoLog());
		debug::debugBreak();
		return false;
	}

	{
		vertexBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*vertexIntermediate, vertexBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Vertex GLSL to SPV:\n%s", allMessages.c_str());
	}

	{
		fragmentBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*fragmentIntermediate, fragmentBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Fragment GLSL to SPV:\n%s", allMessages.c_str());
	}

	return true;
}

void buildProgramReflection(glslang::TProgram* program, PipelineState::Reflection& reflection)
{
	reflection.inputLayout.clear();
	int attribs = program->getNumLiveAttributes();
	for (int attrib = 0; attrib < attribs; ++attrib)
	{
		const char* attribName = program->getAttributeName(attrib);
		VertexAttributeUsage usage = stringToVertexAttributeUsage(attribName);
		if (usage != VertexAttributeUsage::Unknown)
		{
			int attribType = program->getAttributeType(attrib);
			DataType dataType = vulkan::gl::dataTypeFromGLType(attribType);
			reflection.inputLayout.push_back(usage, dataType);
		}
		else
		{
			log::info("Unknown vertex attribute: %s", attribName);
		}
	}

	int blocks = program->getNumLiveUniformBlocks();
	for (int block = 0; block < blocks; ++block)
	{
		String blockName(program->getUniformBlockName(block));
		int blockSize = program->getUniformBlockSize(block);
		if (blockName == PipelineState::kObjectVariables())
		{
			reflection.objectVariablesBufferSize = blockSize;
		}
		else if (blockName == PipelineState::kMaterialVariables())
		{
			reflection.materialVariablesBufferSize = blockSize;
		}
		else if (blockName == PipelineState::kPassVariables())
		{
			reflection.passVariablesBufferSize = blockSize;
		}
		else
		{
			log::error("Unknown uniform block: %s", blockName.c_str());
		}
		int blockIndex = program->getUniformBlockIndex(block);
		int blockOffset = program->getUniformBufferOffset(block);
		// log::info("Block: %s at %d, size: %d", blockName.c_str(), blockIndex, blockSize);
	}

	int uniforms = program->getNumLiveUniformVariables();
	for (int uniform = 0; uniform < uniforms; ++uniform)
	{
		String uniformName(program->getUniformName(uniform));
		int uniformIndex = program->getUniformIndex(uniformName.c_str());
		int uniformType = program->getUniformType(uniform);

		size_t dotPos = uniformName.find(".");
		if (dotPos == String::npos)
		{
			if (vulkan::gl::isSamplerType(uniformType))
			{
				MaterialTexture tex = mtl::stringToMaterialTexture(uniformName);
				const String& samplerName = mtl::materialSamplerToString(tex);

				reflection.vertexTextures.emplace(uniformName, uniformIndex);
				reflection.fragmentTextures.emplace(uniformName, uniformIndex);
				reflection.vertexSamplers.emplace(samplerName, uniformIndex);
				reflection.fragmentSamplers.emplace(samplerName, uniformIndex);
				// log::info("Texture %s, index: %d", uniformName.c_str(), uniformIndex);
			}
			else
			{
				log::error("Unsupported uniform found in program: %s", uniformName.c_str());
			}
		}
		else
		{
			int uniformOffset = program->getUniformBufferOffset(uniform);
			// log::info("Uniform %s, offset: %d, index: %d, type: %x", uniformName.c_str(), uniformOffset, uniformIndex, uniformType);
			String blockName = uniformName.substr(0, dotPos);
			uniformName.erase(0, dotPos + 1);
			if (blockName == PipelineState::kObjectVariables())
			{
				reflection.objectVariables[uniformName].offset = static_cast<uint32_t>(uniformOffset);
			}
			else if (blockName == PipelineState::kMaterialVariables())
			{
				reflection.materialVariables[uniformName].offset = static_cast<uint32_t>(uniformOffset);
			}
			else if (blockName == PipelineState::kPassVariables())
			{
				reflection.passVariables[uniformName].offset = static_cast<uint32_t>(uniformOffset);
			}
			else
			{
				log::error("Unknown uniform block: %s for uniform %s", blockName.c_str(), uniformName.c_str());
			}
		}
	}
}

}
