/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/context.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/base/materiallibrary.h>
#include <et/rendering/interface/databuffer.h>
#include <et/rendering/interface/renderpass.h>
#include <et/rendering/interface/pipelinestate.h>
#include <et/rendering/interface/sampler.h>

namespace et
{
class RenderContext;
class RenderInterface : public Shared
{
public:
	ET_DECLARE_POINTER(RenderInterface);

public:
	RenderInterface(RenderContext* rc)
		: _rc(rc) { }

	virtual ~RenderInterface() = default;

	RenderContext* rc() const
		{ return _rc; }

	MaterialLibrary& sharedMaterialLibrary()
		{ return _sharedMaterialLibrary; }

	ConstantBuffer& sharedConstantBuffer() 
		{ return _sharedConstantBuffer; }

	virtual RenderingAPI api() const = 0;

	virtual void init(const RenderContextParameters& params) = 0;
	virtual void shutdown() = 0;

	virtual void begin() = 0;
	virtual void present() = 0;

	virtual RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) = 0;
	virtual void submitRenderPass(RenderPass::Pointer) = 0;

	/*
	 * Buffers
	 */
	virtual DataBuffer::Pointer createDataBuffer(const std::string&, uint32_t size) = 0;
	virtual DataBuffer::Pointer createDataBuffer(const std::string&, const BinaryDataStorage&) = 0;
	virtual IndexBuffer::Pointer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType) = 0;
	virtual VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexStorage::Pointer, BufferDrawType) = 0;

	/*
	 * Textures
	 */
	virtual Texture::Pointer createTexture(TextureDescription::Pointer) = 0;

	Texture::Pointer loadTexture(const std::string& fileName, ObjectsCache& cache);
	Texture::Pointer defaultTexture();
	
	/*
	 * Programs
	 */
	virtual Program::Pointer createProgram(const std::string& source) = 0;

	/*
	 * Pipeline state
	 */
	virtual PipelineState::Pointer acquirePipelineState(RenderPass::Pointer, Material::Pointer, VertexStream::Pointer) = 0;

	/*
	 * Sampler
	 */
	virtual Sampler::Pointer createSampler(const Sampler::Description&) = 0;
	Sampler::Pointer defaultSampler();

protected:
	void initInternalStructures();
	void shutdownInternalStructures();

private:
	RenderContext* _rc = nullptr;
	MaterialLibrary _sharedMaterialLibrary;
	ConstantBuffer _sharedConstantBuffer;
	Texture::Pointer _defaultTexture;
	Sampler::Pointer _defaultSampler;
};

inline Texture::Pointer RenderInterface::loadTexture(const std::string& fileName, ObjectsCache& cache)
{
	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	if (desc->load(fileName))
	{
		return createTexture(desc);
	}
	log::error("Unable to load texture from %s", fileName.c_str());
	return defaultTexture();
}

inline Texture::Pointer RenderInterface::defaultTexture()
{
	if (_defaultTexture.invalid())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->size = vec2i(4);
		desc->format = TextureFormat::RGBA8;
		desc->data.resize(64);
		uint32_t* data = reinterpret_cast<uint32_t*>(desc->data.data());
		data[ 0] = 0xFF0000FF; data[ 1] = 0xFFFFFFFF; data[ 2] = 0xFF0000FF; data[ 3] = 0xFFFFFFFF;
		data[ 4] = 0xFFFFFFFF; data[ 5] = 0xFF0000FF; data[ 6] = 0xFFFFFFFF; data[ 7] = 0xFF0000FF;
		data[ 8] = 0xFF0000FF; data[ 9] = 0xFFFFFFFF; data[10] = 0xFF0000FF; data[11] = 0xFFFFFFFF;
		data[12] = 0xFFFFFFFF; data[13] = 0xFF0000FF; data[14] = 0xFFFFFFFF; data[15] = 0xFF0000FF;
		_defaultTexture = createTexture(desc);
	}
	return _defaultTexture;
}

inline Sampler::Pointer RenderInterface::defaultSampler()
{
	if (_defaultSampler.invalid())
	{
		Sampler::Description desc;
		_defaultSampler = createSampler(desc);
	}

	return _defaultSampler;
}

inline void RenderInterface::initInternalStructures()
{
	_sharedConstantBuffer.init(this);
	_sharedMaterialLibrary.init(this);
}

inline void RenderInterface::shutdownInternalStructures()
{
	_sharedMaterialLibrary.shutdown();
	_sharedConstantBuffer.shutdown();
}

}
