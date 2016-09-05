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

	MetalState& metal;
    MetalNativePipelineState state;
	MetalNativeBuffer uniforms;
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

	MTLVertexDescriptor* vertexDesc = [MTLVertexDescriptor vertexDescriptor];

	vertexDesc.layouts[0].stride = decl.totalSize();
	vertexDesc.layouts[0].stepRate = 1;
	vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

	NSUInteger index = 0;
	for (const VertexElement& element : decl.elements())
	{
		vertexDesc.attributes[index].format = metal::dataTypeToVertexFormat(element.type());
		vertexDesc.attributes[index].offset = element.offset();
		vertexDesc.attributes[index].bufferIndex = 0;
		++index;
	}

	MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = mtlProgram->nativeProgram().vertexFunction;
    desc.fragmentFunction = mtlProgram->nativeProgram().fragmentFunction;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	desc.depthAttachmentPixelFormat = _private->metal.defaultDepthBuffer.pixelFormat;
    desc.inputPrimitiveTopology = metal::primitiveTypeToTopology(vertexStream()->indexBuffer()->primitiveType());
	desc.vertexDescriptor = vertexDesc;

	NSError* error = nil;
    MTLRenderPipelineReflection* reflection = nil;
    
    _private->state.pipelineState = [_private->metal.device newRenderPipelineStateWithDescriptor:desc
        options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
        reflection:&reflection error:&error];
    
    _private->state.reflection = reflection;

    if (error != nil)
    {
        log::error("Failed to create pipeline:\n%s", [[error description] UTF8String]);
    }

	for (MTLArgument* arg in reflection.vertexArguments)
	{
		if ((arg.type == MTLArgumentTypeBuffer) && [arg.name isEqualToString:@"uniforms"])
		{
			ET_ASSERT(arg.bufferDataType == MTLDataTypeStruct);
			_private->uniforms = MetalNativeBuffer(_private->metal, static_cast<uint32_t>(arg.bufferDataSize));
		}
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
	return _private->uniforms;
}

}
