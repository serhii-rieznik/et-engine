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
#include <external/spirvcross/spirv_msl.hpp>
#include "vulkan_glslang.h"
#include <fstream>

#define ET_COMPILE_TEST_HLSL		  1
#define ET_CROSS_COMPILE_SHADERS_TEST 0

#if (ET_PLATFORM_WIN && ET_COMPILE_TEST_HLSL)
#	include <d3dcompiler.h>
#	include <wrl/client.h>
#	pragma comment(lib, "d3dcompiler.lib")
#endif

namespace glslang 
{
extern const TBuiltInResource DefaultTBuiltInResource;
}

namespace et
{

void buildProgramReflection(glslang::TProgram*, Program::Reflection& reflection);
void dumpSource(const std::string&);
void crossCompile(const std::vector<uint32_t>&);

class VertexShaderAttribLocationTraverser : public glslang::TIntermTraverser
{
	void visitSymbol(glslang::TIntermSymbol* symbol) override
	{
		std::string attribName(symbol->getName().c_str());
		glslang::TQualifier& qualifier = symbol->getQualifier();

		if ((qualifier.storage == glslang::TStorageQualifier::EvqVaryingIn) && qualifier.hasLocation())
		{
			VertexAttributeUsage usage = stringToVertexAttributeUsage(attribName);
			if ((usage != VertexAttributeUsage::Unknown) && (static_cast<int>(usage) != qualifier.layoutLocation))
				qualifier.layoutLocation = static_cast<int>(usage);
		}
	}
};

bool hlslToSPIRV(const std::string& _source, std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin,
	Program::Reflection& reflection)
{
	std::string preprocessedVertexShader;
	std::string preprocessedFragmentShader;

	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgReadHlsl);
	const char* vs[] = { _source.c_str() };
	const char* fs[] = { _source.c_str() };
	const char* vsName[] = { "vertex_shader_source" };
	const char* fsName[] = { "fragment_shader_source" };

	glslang::TShader::ForbidIncluder defaultIncluder;

	glslang::TShader vertexShader(EShLanguage::EShLangVertex);
	vertexShader.setStringsWithLengthsAndNames(vs, nullptr, vsName, 1);
	vertexShader.setAutoMapBindings(true);
	vertexShader.setEntryPoint("vertexMain");
	if (!vertexShader.preprocess(&glslang::DefaultTBuiltInResource, 110, EProfile::ECoreProfile, false, true,
		messages, &preprocessedVertexShader, defaultIncluder))
	{
		log::error("Failed to preprocess vertex shader:\n%s", vertexShader.getInfoLog());
		dumpSource(_source);
		debug::debugBreak();
		return false;
	}
	vs[0] = { preprocessedVertexShader.c_str() };
	vertexShader.setStringsWithLengthsAndNames(vs, nullptr, vsName, 1);

	glslang::TShader fragmentShader(EShLanguage::EShLangFragment);
	fragmentShader.setStringsWithLengthsAndNames(fs, nullptr, fsName, 1);
	fragmentShader.setAutoMapBindings(true);
	fragmentShader.setEntryPoint("fragmentMain");
	if (!fragmentShader.preprocess(&glslang::DefaultTBuiltInResource, 110, EProfile::ECoreProfile, false, true,
		messages, &preprocessedFragmentShader, defaultIncluder))
	{
		log::error("Failed to preprocess fragment shader:\n%s", fragmentShader.getInfoLog());
		dumpSource(_source);
		debug::debugBreak();
		return false;
	}
	fs[0] = { preprocessedFragmentShader.c_str() };
	fragmentShader.setStringsWithLengthsAndNames(fs, nullptr, fsName, 1);

#if (ET_PLATFORM_WIN && ET_COMPILE_TEST_HLSL)
	Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> vertexErrors = nullptr;
	HRESULT vResult = D3DCompile(preprocessedVertexShader.c_str(), preprocessedVertexShader.length(), 
		nullptr, nullptr, nullptr, "vertexMain", "vs_5_1", 
		D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 
		0, vertexBlob.GetAddressOf(), vertexErrors.GetAddressOf());

	if (FAILED(vResult))
	{
		log::error("Compile test of HLSL vertex shader failed");
		if (vertexErrors)
		{
			std::string errorString(reinterpret_cast<const char*>(vertexErrors->GetBufferPointer()), vertexErrors->GetBufferSize());
			log::error("Errors: %s", errorString.c_str());
			dumpSource(preprocessedVertexShader);
		}
	}

	Microsoft::WRL::ComPtr<ID3DBlob> fragmentBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> fragmentErrors = nullptr;
	HRESULT fResult = D3DCompile(preprocessedFragmentShader.c_str(), preprocessedFragmentShader.length(), 
		nullptr, nullptr, nullptr, "fragmentMain", "ps_5_1", 
		D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 
		0, fragmentBlob.GetAddressOf(), fragmentErrors.GetAddressOf());
	if (FAILED(fResult))
	{
		log::error("Compile test of HLSL fragment shader failed");
		if (fragmentErrors)
		{
			std::string errorString(reinterpret_cast<const char*>(fragmentErrors->GetBufferPointer()), fragmentErrors->GetBufferSize());
			log::error("Errors: %s", errorString.c_str());
			dumpSource(preprocessedFragmentShader);
		}
	}
#endif

	if (!vertexShader.parse(&glslang::DefaultTBuiltInResource, 110, true, messages))
	{
		log::error("Failed to parse vertex shader:\n%s", vertexShader.getInfoLog());
		dumpSource(preprocessedVertexShader);
#	if (ET_DEBUG)
		debug::debugBreak();
#	endif
		return false;
	}
	
	if (!fragmentShader.parse(&glslang::DefaultTBuiltInResource, 110, true, messages))
	{
		log::error("Failed to parse fragment shader:\n%s", fragmentShader.getInfoLog());
		dumpSource(preprocessedFragmentShader);
#	if (ET_DEBUG)
		debug::debugBreak();
#	endif
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

		vertexBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*vertexIntermediate, vertexBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Vertex HLSL to SPV:\n%s", allMessages.c_str());

#	if (ET_CROSS_COMPILE_SHADERS_TEST)
		crossCompile(vertexBin);
#	endif
	}

	{
		fragmentBin.reserve(10240);
		spv::SpvBuildLogger logger;
		glslang::GlslangToSpv(*fragmentIntermediate, fragmentBin, &logger);
		std::string allMessages = logger.getAllMessages();
		if (!allMessages.empty())
			log::info("Fragment HLSL to SPV:\n%s", allMessages.c_str());

#	if (ET_CROSS_COMPILE_SHADERS_TEST)
		crossCompile(fragmentBin);
#	endif
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
		int uniformBlockIndex = program->getUniformBlockIndex(uniform);
		int uniformType = program->getUniformType(uniform);

		String blockName(program->getUniformBlockName(uniformBlockIndex));
		if (blockName.empty())
		{
			MaterialTexture tex = stringToMaterialTexture(uniformName);
			if (tex != MaterialTexture::Count)
			{
				uint32_t binding = static_cast<uint32_t>(tex);
				reflection.textures.vertexTextures.emplace(uniformName, binding);
				reflection.textures.fragmentTextures.emplace(uniformName, binding);
			}
			MaterialTexture smp = samplerToMaterialTexture(uniformName);
			if (smp != MaterialTexture::Count)
			{
				uint32_t binding = static_cast<uint32_t>(smp) + MaterialSamplerBindingOffset;
				reflection.textures.vertexSamplers.emplace(uniformName, binding);
				reflection.textures.fragmentSamplers.emplace(uniformName, binding);
			}
		}
		else
		{
			int uniformOffset = program->getUniformBufferOffset(uniform);
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
	spirv_cross::CompilerGLSL compiler(spirv);
	compiler.build_combined_image_samplers();
	dumpSource(compiler.compile());
}

}

namespace glslang
{
const TBuiltInResource DefaultTBuiltInResource = {
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
    }};
}
