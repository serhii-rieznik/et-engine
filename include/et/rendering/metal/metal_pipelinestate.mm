/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal_pipelinestate.h>

namespace et
{

class MetalPipelineStatePrivate
{
public:
	MetalPipelineStatePrivate(MetalState& mtl) :
		metal(mtl) { }

	struct ProgramVariable
	{
		uint32_t offset = 0;
		uint32_t size = 0;
		uint32_t bufferIndex = 0; // vertex/fragment
		uint32_t shader = 0;
	};

	struct BufferDescription
	{
		MetalNativeBuffer::Pointer buffer;
		uint32_t index = InvalidIndex;
		uint32_t size = 0;
		NSRange modifiedRange { };
	};

	MetalState& metal;
    MetalNativePipelineState state;
	BufferDescription vertexVariables;
	BufferDescription fragmentVariables;
	Map<std::string, ProgramVariable> variables;

	void loadVariables(MTLArgument* arg, uint32_t shaderIndex, uint32_t bufferIndex);
};

MetalPipelineState::MetalPipelineState(MetalState& mtl)
{
	ET_PIMPL_INIT(MetalPipelineState, mtl);
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
		if ([arg.name isEqualToString:@"variables"] &&
			(arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			_private->vertexVariables.index = static_cast<uint32_t>(arg.index);
			_private->vertexVariables.size = static_cast<uint32_t>(arg.bufferDataSize);
			_private->loadVariables(arg, (1 << 0), _private->vertexVariables.index);
		}
	}

	for (MTLArgument* arg in reflection.fragmentArguments)
	{
		if ([arg.name isEqualToString:@"variables"] &&
			(arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			_private->fragmentVariables.index = static_cast<uint32_t>(arg.index);
			_private->fragmentVariables.size = static_cast<uint32_t>(arg.bufferDataSize);
			_private->loadVariables(arg, (1 << 1), _private->fragmentVariables.index);
		}
	}

	if (_private->vertexVariables.size > 0)
	{
		_private->vertexVariables.buffer =
			MetalNativeBuffer::Pointer::create(_private->metal, _private->vertexVariables.size);
	}

	if ((_private->fragmentVariables.size > 0) &&
		((_private->fragmentVariables.index != _private->vertexVariables.index) ||
		(_private->fragmentVariables.index != _private->vertexVariables.index)))
	{
		_private->fragmentVariables.buffer =
			MetalNativeBuffer::Pointer::create(_private->metal, _private->fragmentVariables.size);
	}
	else
	{
		_private->fragmentVariables = _private->vertexVariables;
	}

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

const MetalNativeBuffer& MetalPipelineState::vertexVariables() const
{
	return _private->vertexVariables.buffer.reference();
}

const MetalNativeBuffer& MetalPipelineState::fragmentVariables() const
{
	return _private->fragmentVariables.buffer.reference();
}

void MetalPipelineState::uploadProgramVariable(const std::string& name, const void* ptr, uint32_t size)
{
	auto var = _private->variables.find(name);
	if (var == _private->variables.end())
		return;

	MetalPipelineStatePrivate::BufferDescription* desc = nullptr;

	if (var->second.shader & (1 << 0))
	{
		desc = &_private->vertexVariables;
	}
	else if (var->second.shader & (1 << 1))
	{
		desc = &_private->fragmentVariables;
	}
	else
	{
		ET_FAIL("Unable to figure out buffer for variable");
		return;
	}

	uint8_t* bufferData = reinterpret_cast<uint8_t*>([desc->buffer->buffer() contents]);
	memcpy(bufferData + var->second.offset, ptr, size);

	uint32_t endLocation = var->second.offset + size;
	uint32_t currentEndLocation = static_cast<uint32_t>(desc->modifiedRange.location + desc->modifiedRange.length);
	desc->modifiedRange.location = std::min(static_cast<uint32_t>(desc->modifiedRange.location), var->second.offset);
	desc->modifiedRange.length = std::max(endLocation, currentEndLocation) - desc->modifiedRange.location;
}

void MetalPipelineState::bind(MetalNativeEncoder& e)
{
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
	[e.encoder setDepthStencilState:_private->state.depthStencilState];

	if (_private->vertexVariables.buffer.valid() && (_private->vertexVariables.index != InvalidIndex))
	{
		id<MTLBuffer> buffer = _private->vertexVariables.buffer->buffer();
		if (_private->vertexVariables.modifiedRange.length > 0)
		{
			[buffer didModifyRange:_private->vertexVariables.modifiedRange];
		}
		[e.encoder setVertexBuffer:buffer offset:0 atIndex:_private->vertexVariables.index];
	}

	if (_private->fragmentVariables.buffer.valid() && (_private->fragmentVariables.index != InvalidIndex))
	{
		id<MTLBuffer> buffer = _private->fragmentVariables.buffer->buffer();
		if (_private->fragmentVariables.modifiedRange.length > 0)
		{
			[buffer didModifyRange:_private->vertexVariables.modifiedRange];
		}
		[e.encoder setFragmentBuffer:buffer offset:0 atIndex:_private->fragmentVariables.index];
	}
}

/*
 * Private
 */
void MetalPipelineStatePrivate::loadVariables(MTLArgument* arg, uint32_t shaderIndex, uint32_t bufferIndex)
{
	MTLStructType* structType = [arg bufferStructType];
	for (MTLStructMember* member in structType.members)
	{
		std::string name([member.name UTF8String]);
		variables[name].offset = static_cast<uint32_t>(member.offset);
		variables[name].shader |= shaderIndex;
		variables[name].bufferIndex = bufferIndex;
	}
}

}
