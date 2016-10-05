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

	void loadVariables(MTLArgument*, PipelineState::VariableMap&, uint32_t& bufferSize);
	void validateVariables(MTLArgument*, const PipelineState::VariableMap&, uint32_t bufferSize);

	MetalState& metal;
	MetalRenderer* renderer = nullptr;
    MetalNativePipelineState state;

	uint32_t passVariablesBufferSize = 0;

	BinaryDataStorage materialVariablesBuffer;
	uint32_t materialVariablesBufferSize = 0;

	BinaryDataStorage objectVariablesBuffer;
	uint32_t objectVariablesBufferSize = 0;

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
    MTLRenderPipelineReflection* localReflection = nil;
    
    _private->state.pipelineState = [_private->metal.device newRenderPipelineStateWithDescriptor:desc
        options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo reflection:&localReflection error:&error];
	_private->state.reflection = localReflection;
    
    if (error != nil)
    {
        log::error("Failed to create pipeline:\n%s", [[error description] UTF8String]);
    }
	buildReflection();

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

void MetalPipelineState::uploadMaterialVariable(const String& name, const void* ptr, uint32_t size)
{
	auto var = reflection.materialVariables.find(name);
	if (var != reflection.materialVariables.end())
	{
		if (_private->materialVariablesBuffer.empty())
		{
			_private->materialVariablesBuffer.resize(_private->materialVariablesBufferSize);
		}
		auto* dst = _private->materialVariablesBuffer.element_ptr(var->second.offset);
		memcpy(dst, ptr, size);
	}
}

void MetalPipelineState::uploadObjectVariable(const String& name, const void* ptr, uint32_t size)
{
	auto var = reflection.objectVariables.find(name);
	if (var != reflection.objectVariables.end())
	{
		if (_private->objectVariablesBuffer.empty())
		{
			_private->objectVariablesBuffer.resize(_private->objectVariablesBufferSize);
		}
		auto* dst = _private->objectVariablesBuffer.element_ptr(var->second.offset);
		memcpy(dst, ptr, size);
	}
}

void MetalPipelineState::bind(MetalNativeEncoder& e, Material::Pointer material)
{
	for (const auto& p : material->properties())
	{
		const Material::Property& prop = p.second;
		uploadMaterialVariable(p.first, prop.data, prop.length);
	}

	MetalDataBuffer::Pointer sharedBuffer = _private->renderer->sharedConstBuffer().buffer();
	id<MTLBuffer> mtlSharedBuffer = sharedBuffer->nativeBuffer().buffer();

	if (_private->buildMaterialBuffer)
	{
		uint32_t materialBufferOffset = 0;
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(_private->materialVariablesBufferSize, materialBufferOffset);
		memcpy(dst, _private->materialVariablesBuffer.data(), _private->materialVariablesBufferSize);
		if (_private->bindMaterialVariablesToVertex)
			[e.encoder setVertexBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
		if (_private->bindMaterialVariablesToFragment)
			[e.encoder setFragmentBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
	}

	if (_private->buildObjectBuffer)
	{
		uint32_t objectBufferOffset = 0;
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(_private->objectVariablesBufferSize, objectBufferOffset);
		memcpy(dst, _private->objectVariablesBuffer.data(), _private->objectVariablesBufferSize);

		if (_private->bindObjectVariablesToVertex)
			[e.encoder setVertexBuffer:mtlSharedBuffer offset:objectBufferOffset atIndex:ObjectVariablesBufferIndex];
		if (_private->bindObjectVariablesToFragment)
			[e.encoder setFragmentBuffer:mtlSharedBuffer offset:objectBufferOffset atIndex:ObjectVariablesBufferIndex];
	}

	for (const auto& rt : reflection.vertexTextures)
	{
		MetalTexture::Pointer tex = material->texture(rt.first);
		if (tex.invalid())
		{
			tex = _private->renderer->defaultTexture();
		}
		[e.encoder setVertexTexture:tex->nativeTexture().texture atIndex:rt.second];
	}

	for (const auto& rt : reflection.fragmentTextures)
	{
		MetalTexture::Pointer tex = material->texture(rt.first);
		if (tex.invalid())
		{
			tex = _private->renderer->defaultTexture();
		}
		[e.encoder setFragmentTexture:tex->nativeTexture().texture atIndex:rt.second];
	}

	[e.encoder setDepthStencilState:_private->state.depthStencilState];
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
}

void MetalPipelineState::buildReflection()
{
	for (MTLArgument* arg in _private->state.reflection.vertexArguments)
	{
		if (arg.active && (arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			if (arg.index == PassVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.passVariables, _private->passVariablesBufferSize);
			}
			else if (arg.index == MaterialVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.materialVariables, _private->materialVariablesBufferSize);
				_private->bindMaterialVariablesToVertex = true;
			}
			else if (arg.index == ObjectVariablesBufferIndex)
			{
				_private->loadVariables(arg, reflection.objectVariables, _private->objectVariablesBufferSize);
				_private->bindObjectVariablesToVertex = true;
			}
		}
		else if (arg.active && (arg.type == MTLArgumentTypeTexture))
		{
			String argName([arg.name UTF8String]);
			reflection.vertexTextures[argName] = static_cast<uint32_t>(arg.index);
		}
	}

	for (MTLArgument* arg in _private->state.reflection.fragmentArguments)
	{
		if (arg.active && (arg.type == MTLArgumentTypeBuffer) && (arg.bufferDataType == MTLDataTypeStruct))
		{
			if (arg.index == PassVariablesBufferIndex)
			{
				if (_private->passVariablesBufferSize > 0)
					_private->validateVariables(arg, reflection.passVariables, _private->passVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.passVariables, _private->passVariablesBufferSize);
			}
			else if (arg.index == MaterialVariablesBufferIndex)
			{
				if (_private->bindMaterialVariablesToVertex)
					_private->validateVariables(arg, reflection.materialVariables, _private->materialVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.materialVariables, _private->materialVariablesBufferSize);
				_private->bindMaterialVariablesToFragment = true;
			}
			else if (arg.index == ObjectVariablesBufferIndex)
			{
				if (_private->bindObjectVariablesToVertex)
					_private->validateVariables(arg, reflection.objectVariables, _private->objectVariablesBufferSize);
				else
					_private->loadVariables(arg, reflection.objectVariables, _private->objectVariablesBufferSize);
				_private->bindObjectVariablesToVertex = true;
			}
		}
		else if (arg.active && (arg.type == MTLArgumentTypeTexture))
		{
			String argName([arg.name UTF8String]);
			reflection.fragmentTextures[argName] = static_cast<uint32_t>(arg.index);
		}
	}

	printReflection();

	_private->buildMaterialBuffer = _private->bindMaterialVariablesToVertex || _private->bindMaterialVariablesToFragment;
	_private->buildObjectBuffer = _private->bindObjectVariablesToVertex || _private->bindObjectVariablesToFragment;
}

/*
 * Private
 */
void MetalPipelineStatePrivate::loadVariables(MTLArgument* arg, PipelineState::VariableMap& variables, uint32_t& bufferSize)
{
	bufferSize = static_cast<uint32_t>(arg.bufferDataSize);
	MTLStructType* structType = [arg bufferStructType];
	for (MTLStructMember* member in structType.members)
	{
		String name([member.name UTF8String]);
		variables[name].offset = static_cast<uint32_t>(member.offset);
	}
}

void MetalPipelineStatePrivate::validateVariables(MTLArgument* arg, const PipelineState::VariableMap& variables, uint32_t bufferSize)
{
	ET_ASSERT(arg.bufferDataSize == bufferSize);
	MTLStructType* structType = [arg bufferStructType];
	for (MTLStructMember* member in structType.members)
	{
		String name([member.name UTF8String]);
		ET_ASSERT(variables.count(name) > 0);
		ET_ASSERT(variables.at(name).offset == member.offset);
	}
}

}
