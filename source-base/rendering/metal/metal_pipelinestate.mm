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

	MetalState& metal;
    MetalNativePipelineState state;
	MetalNativeBuffer::Pointer vertexVariables;
	MetalNativeBuffer::Pointer fragmentVariables;
	Map<std::string, ProgramVariable> variables;

	uint32_t vertexBufferIndex = static_cast<uint32_t>(-1);
	uint32_t vertexBufferSize = 0;

	uint32_t fragmentBufferIndex = static_cast<uint32_t>(-1);
	uint32_t fragmentBufferSize = 0;

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
			_private->vertexBufferIndex = static_cast<uint32_t>(arg.index);
			_private->vertexBufferSize = static_cast<uint32_t>(arg.bufferDataSize);
			_private->loadVariables(arg, (1 << 0), _private->vertexBufferIndex);
		}
	}

	for (MTLArgument* arg in reflection.fragmentArguments)
	{
		if ([arg.name isEqualToString:@"variables"] &&
			(arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			_private->fragmentBufferIndex = static_cast<uint32_t>(arg.index);
			_private->fragmentBufferSize = static_cast<uint32_t>(arg.bufferDataSize);
			_private->loadVariables(arg, (1 << 1), _private->fragmentBufferIndex);
		}
	}

	if (_private->vertexBufferSize > 0)
	{
		_private->vertexVariables = MetalNativeBuffer::Pointer::create(_private->metal, _private->vertexBufferSize);
	}

	if ((_private->fragmentBufferSize > 0) &&
		((_private->fragmentBufferIndex != _private->vertexBufferIndex) ||
		(_private->fragmentBufferIndex != _private->vertexBufferIndex)))
	{
		_private->fragmentVariables = MetalNativeBuffer::Pointer::create(_private->metal, _private->fragmentBufferSize);
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

const MetalNativeBuffer& MetalPipelineState::uniformsBuffer() const
{
	return _private->vertexVariables.reference();
}

void MetalPipelineState::uploadProgramVariable(const std::string& name, const void* ptr, uint32_t size)
{
	auto var = _private->variables.find(name);
	if (var == _private->variables.end())
		return;

	id<MTLBuffer> buffer = nil;

	if (var->second.shader & (1 << 0))
	{
		buffer = _private->vertexVariables->buffer();
	}
	else if (var->second.shader & (1 << 1))
	{
		buffer = _private->fragmentVariables->buffer();
	}
	else
	{
		ET_FAIL("Unable to figure out buffer for variable");
		return;
	}

	uint8_t* bufferData = reinterpret_cast<uint8_t*>([buffer contents]);
	memcpy(bufferData + var->second.offset, ptr, size);
	[buffer didModifyRange:NSMakeRange(var->second.offset, size)];
}

void MetalPipelineState::bind(MetalNativeEncoder& e)
{
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
	[e.encoder setDepthStencilState:_private->state.depthStencilState];

	if (_private->vertexVariables.valid())
	{
		[e.encoder setVertexBuffer:_private->vertexVariables->buffer() offset:0 atIndex:_private->vertexBufferIndex];
	}

	if (_private->fragmentVariables.valid())
	{
		[e.encoder setFragmentBuffer:_private->fragmentVariables->buffer() offset:0 atIndex:_private->fragmentBufferIndex];
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
