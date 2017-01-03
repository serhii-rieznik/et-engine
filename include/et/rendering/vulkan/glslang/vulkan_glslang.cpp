/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <external/glslang/glslang/Public/ShaderLang.h>
#include <external/glslang/SPIRV/Logger.h>
#include <external/glslang/SPIRV/GlslangToSpv.h>
#include <external/glslang/OGLCompilersDLL/InitializeDll.h>
#include <external/glslang/glslang/MachineIndependent/localintermediate.h>
#include <external/spirvcross/spirv_cross.hpp>
#include <external/spirvcross/spirv_glsl.hpp>
#include <et/core/et.h>
#include <functional>
#include "vulkan_glslang.h"

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

void buildProgramReflection(glslang::TProgram*, Program::Reflection& reflection);
void dumpSource(const std::string&);
void crossCompile(const std::vector<uint32_t>&);

bool glslToSPIRV(const std::string& vertexSource, const std::string& fragmentSource,
	std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin, Program::Reflection& reflection)
{
	glslang::TProgram* program = nullptr;
	EShMessages messages = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules);
	const char* vertexSourceCStr[] = { vertexSource.c_str() };
	const char* vertexFileNames[] = { "Vertex Shader" };
	const char* fragmentSourceCStr[] = { fragmentSource.c_str() };
	const char* fragmentFileNames[] = { "Fragment Shader" };

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
	});

	if (!vertexShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse vertex shader:\n%s", vertexShader.getInfoLog());
		dumpSource(vertexSource);
		debug::debugBreak();
		return false;
	}
	
	if (!fragmentShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse fragment shader:\n%s", fragmentShader.getInfoLog());
		dumpSource(fragmentSource);
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

class VertexShaderAttribLocationTraverser : public glslang::TIntermTraverser
{
	void visitSymbol(glslang::TIntermSymbol* symbol) override
	{
		glslang::TQualifier& qualifier = symbol->getQualifier();
		if ((qualifier.storage == glslang::TStorageQualifier::EvqVaryingIn) && qualifier.hasLocation())
		{
			std::string attribName(symbol->getName().c_str());
			VertexAttributeUsage usage = stringToVertexAttributeUsage(attribName);
			if (usage != VertexAttributeUsage::Unknown)
			{
				log::info("Input symbol: %s, location remapped from %d to %d", 
					attribName.c_str(), qualifier.layoutLocation, static_cast<uint32_t>(usage));
				// qualifier.layoutLocation = static_cast<int>(usage);
			}
		}
	}
};

bool hlslToSPIRV(const std::string& source, std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin,
	Program::Reflection& reflection)
{
	EShMessages messages = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules | EShMsgReadHlsl);
	const char* sourceCStr[] = { source.c_str(), source.c_str() };
	const char* sourceNames[] = { "vertex_shader_source", "fragment_shader_source" };

	glslang::TShader vertexShader(EShLanguage::EShLangVertex);
	vertexShader.setEntryPoint("vertexMain");
	vertexShader.setAutoMapBindings(true);
	vertexShader.setStringsWithLengthsAndNames(sourceCStr, nullptr, sourceNames, 1);
	if (!vertexShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse vertex shader:\n%s", vertexShader.getInfoLog());
		dumpSource(source);
		debug::debugBreak();
		return false;
	}
	
	glslang::TShader fragmentShader(EShLanguage::EShLangFragment);
	fragmentShader.setEntryPoint("fragmentMain");
	fragmentShader.setStringsWithLengthsAndNames(sourceCStr + 1, nullptr, sourceNames + 1, 1);
	fragmentShader.setAutoMapBindings(true);
	if (!fragmentShader.parse(&defaultBuiltInResource, 100, true, messages))
	{
		log::error("Failed to parse fragment shader:\n%s", fragmentShader.getInfoLog());
		dumpSource(source);
		debug::debugBreak();
		return false;
	}
	
	std::unique_ptr<glslang::TProgram> program = std::make_unique<glslang::TProgram>();
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

	buildProgramReflection(program.get(), reflection);

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
		VertexShaderAttribLocationTraverser attribFixup;
		vertexIntermediate->getTreeRoot()->traverse(&attribFixup);

		// trav.
		vertexBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*vertexIntermediate, vertexBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Vertex HLSL to SPV:\n%s", allMessages.c_str());
		crossCompile(vertexBin);
	}

	{
		fragmentBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*fragmentIntermediate, fragmentBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Fragment HLSL to SPV:\n%s", allMessages.c_str());
		crossCompile(fragmentBin);
	}

	return true;
}

void initGlslangResources()
{
	glslang::InitializeProcess();
}

void cleanupGlslangResources()
{
	glslang::FinalizeProcess();
}

void buildProgramReflection(glslang::TProgram* program, Program::Reflection& reflection)
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
	}

	int uniforms = program->getNumLiveUniformVariables();
	for (int uniform = 0; uniform < uniforms; ++uniform)
	{
		String uniformName(program->getUniformName(uniform));
		int uniformType = program->getUniformType(uniform);

		size_t dotPos = uniformName.find(".");
		if (dotPos == String::npos)
		{
			if (vulkan::gl::isSamplerType(uniformType))
			{
				MaterialTexture tex = mtl::stringToMaterialTexture(uniformName);
				const String& samplerName = mtl::materialSamplerToString(tex);

				uint32_t binding = static_cast<uint32_t>(tex);
				reflection.textures.vertexTextures.emplace(uniformName, binding);
				reflection.textures.fragmentTextures.emplace(uniformName, binding);
				reflection.textures.vertexSamplers.emplace(samplerName, binding);
				reflection.textures.fragmentSamplers.emplace(samplerName, binding);
			}
			else
			{
				log::error("Unsupported uniform found in program: %s", uniformName.c_str());
			}
		}
		else
		{
			int uniformOffset = program->getUniformBufferOffset(uniform);
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

void dumpSource(const std::string& s)
{
	StringList lines = split(s, "\n");

	uint32_t lineIndex = 1;
	for (const std::string& line : lines)
	{
		log::info("%04u: %s", lineIndex, line.c_str());
		++lineIndex;
	}
}

void crossCompile(const std::vector<uint32_t>& spirv)
{
	std::unique_ptr<spirv_cross::Compiler> compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv);
	std::string glsl = compiler->compile();
	dumpSource(glsl);
}

}
