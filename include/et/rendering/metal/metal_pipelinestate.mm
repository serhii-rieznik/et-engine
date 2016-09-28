/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderer.h>
#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal_texture.h>
#include <et/rendering/metal/metal_pipelinestate.h>

namespace et
{

class MetalPipelineStatePrivate
{
public:
	MetalPipelineStatePrivate(MetalState& mtl, MetalRenderer* ren) :
		metal(mtl), renderer(ren) { }

	void loadVariables(MTLArgument* arg);
	void validateVariables(MTLArgument* arg);

	struct ProgramVariable
	{
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	MetalState& metal;
	MetalRenderer* renderer = nullptr;
    MetalNativePipelineState state;

	Map<String, ProgramVariable> variables;
	BinaryDataStorage variablesBuffer;
	uint32_t variablesBufferSize = 0;

	bool bindVariablesToVertex = false;
	bool bindVariablesToFragment = false;
	bool buildConstBuffer = false;
};

MetalPipelineState::MetalPipelineState(MetalRenderer* renderer, MetalState& mtl)
{
	ET_PIMPL_INIT(MetalPipelineState, mtl, renderer);
}

MetalPipelineState::~MetalPipelineState()
{
    ET_OBJC_RELEASE(_private->state.depthStencilState);
    ET_OBJC_RELEASE(_private->state.pipelineState);
    ET_PIMPL_FINALIZE(MetalPipelineState);
}

void MetalPipelineState::build()
{
    MetalProgram::Pointer mtlProgram = program();
    const VertexDeclaration& decl = inputLayout();

	MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
	desc.vertexFunction = mtlProgram->nativeProgram().vertexFunction;
	desc.fragmentFunction = mtlProgram->nativeProgram().fragmentFunction;
	desc.colorAttachments[0].pixelFormat = metal::renderableTextureFormatValue(renderTargetFormat());
	desc.depthAttachmentPixelFormat = _private->metal.defaultDepthBuffer.pixelFormat;
	desc.inputPrimitiveTopology = metal::primitiveTypeToTopology(vertexStream()->indexBuffer()->primitiveType());
	desc.vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	desc.vertexDescriptor.layouts[0].stride = decl.totalSize();
	desc.vertexDescriptor.layouts[0].stepRate = 1;

	NSUInteger index = 0;
	for (const VertexElement& element : decl.elements())
	{
		desc.vertexDescriptor.attributes[index].format = metal::dataTypeToVertexFormat(element.type());
		desc.vertexDescriptor.attributes[index].offset = element.offset();
		desc.vertexDescriptor.attributes[index].bufferIndex = 0;
		++index;
	}

	NSError* error = nil;
    MTLRenderPipelineReflection* reflection = nil;
    
    _private->state.pipelineState = [_private->metal.device newRenderPipelineStateWithDescriptor:desc
        options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
        reflection:&reflection error:&error];
    
    _private->state.reflection = reflection;

	for (MTLArgument* arg in reflection.vertexArguments)
	{
		if ((arg.index == ProgramSpecificBufferIndex) && (arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			_private->loadVariables(arg);
			_private->bindVariablesToVertex = true;
		}
	}

	for (MTLArgument* arg in reflection.fragmentArguments)
	{
		if ((arg.index == ProgramSpecificBufferIndex) && (arg.type == MTLArgumentTypeBuffer) &&
			(arg.bufferDataType == MTLDataTypeStruct))
		{
			if (_private->bindVariablesToVertex)
			{
				_private->validateVariables(arg);
			}
			else
			{
				_private->loadVariables(arg);
			}
			_private->bindVariablesToFragment = true;
		}
	}

	_private->buildConstBuffer = _private->bindVariablesToVertex || _private->bindVariablesToFragment;

    if (error != nil)
    {
        log::error("Failed to create pipeline:\n%s", [[error description] UTF8String]);
    }

    MTLDepthStencilDescriptor* dsDesc = [[MTLDepthStencilDescriptor alloc] init];
    dsDesc.depthWriteEnabled = depthState().depthWriteEnabled;
	dsDesc.depthCompareFunction = metal::compareFunctionValue(depthState().compareFunction);
    _private->state.depthStencilState = [_private->metal.device newDepthStencilStateWithDescriptor:dsDesc];
    
    ET_OBJC_RELEASE(desc);
    ET_OBJC_RELEASE(dsDesc);
}
    
const MetalNativePipelineState& MetalPipelineState::nativeState() const
{
    return _private->state;
}

void MetalPipelineState::uploadProgramVariable(const String& name, const void* ptr, uint32_t size)
{
	auto var = _private->variables.find(name);
	if (var != _private->variables.end())
	{
		if (_private->variablesBuffer.empty())
		{
			_private->variablesBuffer.resize(_private->variablesBufferSize);
		}
		auto* dst = _private->variablesBuffer.element_ptr(var->second.offset);
		memcpy(dst, ptr, size);
	}
}

void MetalPipelineState::bind(MetalNativeEncoder& e, Material::Pointer material)
{
	for (const auto& p : material->properties())
	{
		const Material::Property& prop = p.second;
		uploadProgramVariable(p.first, prop.data, prop.length);
	}

	// TODO : set textures
	for (auto tex : material->textures())
	{

	}

	uint32_t sharedBufferOffset = 0;
	MetalDataBuffer::Pointer sharedBuffer = _private->renderer->sharedConstBuffer().buffer();
	id<MTLBuffer> mtlSharedBuffer = sharedBuffer->nativeBuffer().buffer();
	if (_private->buildConstBuffer)
	{
		uint8_t* dst = nullptr;
		_private->renderer->sharedConstBuffer().allocateData(_private->variablesBufferSize, &dst, sharedBufferOffset);
		memcpy(dst, _private->variablesBuffer.data(), _private->variablesBufferSize);
	}

	if (_private->bindVariablesToVertex)
	{
		[e.encoder setVertexBuffer:mtlSharedBuffer offset:sharedBufferOffset atIndex:ProgramSpecificBufferIndex];
	}

	if (_private->bindVariablesToFragment)
	{
		[e.encoder setFragmentBuffer:mtlSharedBuffer offset:sharedBufferOffset atIndex:ProgramSpecificBufferIndex];
	}

	[e.encoder setDepthStencilState:_private->state.depthStencilState];
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
}

/*
 * Private
 */
void MetalPipelineStatePrivate::loadVariables(MTLArgument* arg)
{
	variablesBufferSize = static_cast<uint32_t>(arg.bufferDataSize);
	MTLStructType* structType = [arg bufferStructType];
	for (MTLStructMember* member in structType.members)
	{
		String name([member.name UTF8String]);
		variables[name].offset = static_cast<uint32_t>(member.offset);
	}
}

void MetalPipelineStatePrivate::validateVariables(MTLArgument* arg)
{
	ET_ASSERT(arg.bufferDataSize == variablesBufferSize);
	MTLStructType* structType = [arg bufferStructType];
	for (MTLStructMember* member in structType.members)
	{
		String name([member.name UTF8String]);
		ET_ASSERT(variables.count(name) > 0);
		ET_ASSERT(variables[name].offset == member.offset);
	}
}

}
