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
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    desc.inputPrimitiveTopology = metal::primitiveTypeToTopology(vertexStream()->indexBuffer()->primitiveType());
    desc.vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    desc.vertexDescriptor.layouts[0].stride = decl.totalSize();
    desc.vertexDescriptor.layouts[0].stepRate = 1;
    desc.vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    uint32_t index = 0;
    for (const VertexElement& element : decl.elements())
    {
        desc.vertexDescriptor.attributes[index].format = metal::dataTypeToVertexFormat(element.type());
        desc.vertexDescriptor.attributes[index].offset = element.offset();
        desc.vertexDescriptor.attributes[index].bufferIndex = 0;
    }
    
    NSError* error = nil;
    _private->state.pipelineState = [_private->metal.device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (error != nil)
    {
        log::error("Failed to create pipeline:\n%s", [[error description] UTF8String]);
    }
    
    MTLDepthStencilDescriptor* dsDesc = [[MTLDepthStencilDescriptor alloc] init];
    dsDesc.depthWriteEnabled = depthState().depthWriteEnabled;
    dsDesc.depthCompareFunction = MTLCompareFunctionAlways;
    _private->state.depthStencilState = [_private->metal.device newDepthStencilStateWithDescriptor:dsDesc];
    
    ET_OBJC_RELEASE(desc);
    ET_OBJC_RELEASE(dsDesc);
}
    
const MetalNativePipelineState& MetalPipelineState::nativeState() const
{
    return _private->state;
}

}
