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
#include <et/rendering/metal/metal_sampler.h>
#include <et/rendering/metal/metal_pipelinestate.h>

namespace et
{

class MetalPipelineStatePrivate
{
public:
	MetalPipelineStatePrivate(MetalState& mtl, MetalRenderer* ren) :
		metal(mtl), renderer(ren) { }

	MetalState& metal;
	MetalRenderer* renderer = nullptr;
    MetalNativePipelineState state;

    // TODO : move to material
    BinaryDataStorage materialVariablesBuffer;

	bool bindMaterialVariablesToVertex = false;
	bool bindMaterialVariablesToFragment = false;
	bool buildMaterialBuffer = false;

	bool bindObjectVariablesToVertex = false;
	bool bindObjectVariablesToFragment = false;
	bool buildObjectBuffer = false;
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

void MetalPipelineState::build(const RenderPass::Pointer&)
{
	/*
	 * TODO : completely rewrite
	 *
	MetalProgram::Pointer mtlProgram = program();
	const VertexDeclaration& providedLayout = inputLayout();

#if ET_DEBUG
	buildRequiredLayout(program()->reflection().inputLayout);
	for (const VertexElement& requiredElement : program()->reflection().inputLayout.elements())
	{
		ET_ASSERT(providedLayout.has(requiredElement.usage()));
	}
#endif

	MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
	desc.vertexFunction = mtlProgram->nativeProgram().vertexFunction;
	desc.fragmentFunction = mtlProgram->nativeProgram().fragmentFunction;
	desc.colorAttachments[0].pixelFormat = metal::renderableTextureFormatValue(renderTargetFormat());
	desc.depthAttachmentPixelFormat = _private->metal.defaultDepthBuffer.pixelFormat;
	desc.inputPrimitiveTopology = metal::primitiveTypeToTopology(primitiveType());

	desc.vertexDescriptor.layouts[VertexStreamBufferIndex].stepFunction = MTLVertexStepFunctionPerVertex;
	desc.vertexDescriptor.layouts[VertexStreamBufferIndex].stride = providedLayout.totalSize();
	desc.vertexDescriptor.layouts[VertexStreamBufferIndex].stepRate = 1;
	for (const VertexElement& element : providedLayout.elements())
	{
		NSUInteger index = static_cast<NSUInteger>(element.usage());
		desc.vertexDescriptor.attributes[index].format = metal::dataTypeToVertexFormat(element.type());
		desc.vertexDescriptor.attributes[index].offset = element.offset();
		desc.vertexDescriptor.attributes[index].bufferIndex = VertexStreamBufferIndex;
	}

	NSError* error = nil;
    MTLRenderPipelineReflection* localReflection = nil;
    
    _private->state.pipelineState = [_private->metal.device newRenderPipelineStateWithDescriptor:desc
        options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo reflection:&localReflection error:&error];
	_private->state.reflection = localReflection;
    
    if (error != nil)
    {
        log::error("Failed to create pipeline:\n%s", [[error description] UTF8String]);
		debug::debugBreak();
    }
	buildReflection();

    MTLDepthStencilDescriptor* dsDesc = [[MTLDepthStencilDescriptor alloc] init];
    dsDesc.depthWriteEnabled = depthState().depthWriteEnabled;
	dsDesc.depthCompareFunction = metal::compareFunctionValue(depthState().compareFunction);
    _private->state.depthStencilState = [_private->metal.device newDepthStencilStateWithDescriptor:dsDesc];
    
    ET_OBJC_RELEASE(desc);
    ET_OBJC_RELEASE(dsDesc);
	// */
}
    
const MetalNativePipelineState& MetalPipelineState::nativeState() const
{
    return _private->state;
}


void MetalPipelineState::bind(MetalNativeEncoder&, MaterialInstance::Pointer /* material */)
{
	/*
	 * TODO : completely rewrite
	 *
	for (const auto& prop : material->usedProperties())
	{
		uploadMaterialVariable(prop.first, prop.second.data, prop.second.size);
	}

	MetalBuffer::Pointer sharedBuffer = _private->renderer->sharedConstBuffer().buffer();
	id<MTLBuffer> mtlSharedBuffer = sharedBuffer->nativeBuffer().buffer();

	if (_private->buildMaterialBuffer)
	{
		uint32_t materialBufferOffset = 0;
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(reflection.materialVariablesBufferSize, materialBufferOffset);
		memcpy(dst, _private->materialVariablesBuffer.data(), program()->reflection().materialVariablesBufferSize);
		if (_private->bindMaterialVariablesToVertex)
			[e.encoder setVertexBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
		if (_private->bindMaterialVariablesToFragment)
			[e.encoder setFragmentBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
	}

	if (_private->buildObjectBuffer)
	{
		uint32_t objectBufferOffset = 0;
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(reflection.objectVariablesBufferSize, objectBufferOffset);
		memcpy(dst, objectVariablesBuffer.data(), reflection.objectVariablesBufferSize);

		if (_private->bindObjectVariablesToVertex)
			[e.encoder setVertexBuffer:mtlSharedBuffer offset:objectBufferOffset atIndex:ObjectVariablesBufferIndex];

		if (_private->bindObjectVariablesToFragment)
			[e.encoder setFragmentBuffer:mtlSharedBuffer offset:objectBufferOffset atIndex:ObjectVariablesBufferIndex];
	}

	for (const auto& vt : reflection.vertexTextures)
	{
		auto i = material->usedTextures().find(vt.first);
		MetalTexture::Pointer tex;
		if (i == material->usedTextures().end())
			tex = _private->renderer->defaultTexture();
		else
			tex = i->second.texture;
		[e.encoder setVertexTexture:tex->nativeTexture().texture atIndex:vt.second];
	}

	for (const auto& ft : reflection.fragmentTextures)
	{
		auto i = material->usedTextures().find(ft.first);
		MetalTexture::Pointer tex;
		if (i == material->usedTextures().end())
			tex = _private->renderer->defaultTexture();
		else
			tex = i->second.texture;
		[e.encoder setFragmentTexture:tex->nativeTexture().texture atIndex:ft.second];
	}

	for (const auto& vs : reflection.vertexSamplers)
	{
		auto i = material->usedSamplers().find(vs.first);
		MetalSampler::Pointer smp = (i == material->usedSamplers().end()) ?
			_private->renderer->defaultSampler() : i->second.sampler;
		[e.encoder setVertexSamplerState:smp->nativeSampler().sampler atIndex:vs.second];
	}

	for (const auto& fs : reflection.fragmentSamplers)
	{
		auto i = material->usedSamplers().find(fs.first);
		MetalSampler::Pointer smp = (i == material->usedSamplers().end()) ?
			_private->renderer->defaultSampler() : i->second.sampler;
		[e.encoder setFragmentSamplerState:smp->nativeSampler().sampler atIndex:fs.second];
	}

	[e.encoder setDepthStencilState:_private->state.depthStencilState];
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
	// */
}

void MetalPipelineState::buildReflection()
{
	/*
	 * TODO : remove, reflection will be available in program
	 *
	for (MTLArgument* arg in _private->state.reflection.vertexArguments)
	{
		if (arg.active && (arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			if (arg.index == PassVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.passVariables, reflection.passVariablesBufferSize);
			}
			else if (arg.index == MaterialVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.materialVariables, reflection.materialVariablesBufferSize);
				_private->bindMaterialVariablesToVertex = true;
			}
			else if (arg.index == ObjectVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.objectVariables, reflection.objectVariablesBufferSize);
				_private->bindObjectVariablesToVertex = true;
			}
		}
		else if (arg.active && (arg.type == MTLArgumentTypeTexture))
		{
			String argName([arg.name UTF8String]);
			reflection.vertexTextures[argName] = static_cast<uint32_t>(arg.index);
		}
		else if (arg.active && (arg.type == MTLArgumentTypeSampler))
		{
			String argName([arg.name UTF8String]);
			reflection.vertexSamplers[argName] = static_cast<uint32_t>(arg.index);
		}
	}

	for (MTLArgument* arg in _private->state.reflection.fragmentArguments)
	{
		if (arg.active && (arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			if (arg.index == PassVariablesBufferIndex)
			{
				if (reflection.passVariablesBufferSize > 0)
					_private->validateVariables(arg, reflection.passVariables, reflection.passVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.passVariables, reflection.passVariablesBufferSize);
			}
			else if (arg.index == MaterialVariablesBufferIndex)
			{
				if (_private->bindMaterialVariablesToVertex)
					_private->validateVariables(arg, reflection.materialVariables, reflection.materialVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.materialVariables, reflection.materialVariablesBufferSize);
				_private->bindMaterialVariablesToFragment = true;
			}
			else if (arg.index == ObjectVariablesBufferIndex)
			{
				if (_private->bindObjectVariablesToVertex)
					_private->validateVariables(arg, reflection.objectVariables, reflection.objectVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.objectVariables, reflection.objectVariablesBufferSize);
				_private->bindObjectVariablesToVertex = true;
			}
		}
		else if (arg.active && (arg.type == MTLArgumentTypeTexture))
		{
			String argName([arg.name UTF8String]);
			reflection.fragmentTextures[argName] = static_cast<uint32_t>(arg.index);
		}
		else if (arg.active && (arg.type == MTLArgumentTypeSampler))
		{
			String argName([arg.name UTF8String]);
			reflection.fragmentSamplers[argName] = static_cast<uint32_t>(arg.index);
		}
	}

	printReflection();

	_private->buildMaterialBuffer = _private->bindMaterialVariablesToVertex || _private->bindMaterialVariablesToFragment;
	if (_private->buildMaterialBuffer)
	{
		_private->materialVariablesBuffer.resize(alignUpTo(reflection.materialVariablesBufferSize, 32));
		_private->materialVariablesBuffer.fill(0);
	}

	_private->buildObjectBuffer = _private->bindObjectVariablesToVertex || _private->bindObjectVariablesToFragment;
	if (_private->buildObjectBuffer)
	{
		objectVariablesBuffer.resize(alignUpTo(reflection.objectVariablesBufferSize, 32));
		objectVariablesBuffer.fill(0);
	}
	*/
}

void MetalPipelineState::buildRequiredLayout(VertexDeclaration& /* decl */)
{
	/*
	 * TODO : remove, layout will be in program
	 *
	decl.clear();

	MetalProgram::Pointer mtlProgram = program();
	id<MTLFunction> vertexFunction = mtlProgram->nativeProgram().vertexFunction;
	for (MTLVertexAttribute* attrib in vertexFunction.vertexAttributes)
	{
		std::string name([attrib.name UTF8String]);
		VertexAttributeUsage usage = stringToVertexAttributeUsage(name);
		if (usage != VertexAttributeUsage::Unknown)
		{
			DataType type = metal::mtlDataTypeToDataType(attrib.attributeType);
			decl.push_back(usage, type);
		}
	}
	*/
}

}
