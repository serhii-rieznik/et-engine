/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/context.h>
#include <et/core/objectscache.h>
#include <et/imaging/texturedescription.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/base/materiallibrary.h>
#include <et/rendering/interface/buffer.h>
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
	virtual void destroy() = 0;

	virtual void resize(const vec2i&) = 0;

	virtual void begin() = 0;
	virtual void present() = 0;

	virtual RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) = 0;
	virtual void submitRenderPass(RenderPass::Pointer) = 0;

	/*
	 * Buffers
	 */
	virtual Buffer::Pointer createBuffer(const std::string&, const Buffer::Description&) = 0;

	Buffer::Pointer createDataBuffer(const std::string&, uint32_t size);
	Buffer::Pointer createDataBuffer(const std::string&, const BinaryDataStorage&);
	Buffer::Pointer createIndexBuffer(const std::string&, const IndexArray::Pointer&, Buffer::Location);
	Buffer::Pointer createVertexBuffer(const std::string&, const VertexStorage::Pointer&, Buffer::Location);

	/*
	 * Textures
	 */
	virtual Texture::Pointer createTexture(const TextureDescription::Pointer&) = 0;
	virtual TextureSet::Pointer createTextureSet(const TextureSet::Description&) = 0;

	Texture::Pointer loadTexture(const std::string& fileName, ObjectsCache& cache);
	Texture::Pointer checkersTexture();
	Texture::Pointer whiteTexture();
	Texture::Pointer blackTexture();
	Texture::Pointer generateHammersleySet(uint32_t size);
	
	/*
	 * Programs
	 */
	virtual Program::Pointer createProgram(const std::string& source) = 0;

	/*
	 * Pipeline state
	 */
	virtual PipelineState::Pointer acquirePipelineState(const RenderPass::Pointer&, const Material::Pointer&, const VertexStream::Pointer&) = 0;

	/*
	 * Sampler
	 */
	virtual Sampler::Pointer createSampler(const Sampler::Description&) = 0;
	Sampler::Pointer defaultSampler();
	Sampler::Pointer clampSampler();
	Sampler::Pointer nearestSampler();

protected:
	void initInternalStructures();
	void shutdownInternalStructures();

private:
	RenderContext* _rc = nullptr;
	MaterialLibrary _sharedMaterialLibrary;
	ConstantBuffer _sharedConstantBuffer;
	Texture::Pointer _checkersTexture;
	Texture::Pointer _whiteTexture;
	Texture::Pointer _blackTexture;
	Sampler::Pointer _defaultSampler;
	Sampler::Pointer _nearestSampler;
	Sampler::Pointer _clampSampler;
};

inline Texture::Pointer RenderInterface::loadTexture(const std::string& fileName, ObjectsCache& cache)
{
	LoadableObject::Collection existingObjects = cache.findObjects(fileName);
	if (existingObjects.empty())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		if (desc->load(fileName))
		{
			Texture::Pointer texture = createTexture(desc);
			texture->setOrigin(fileName);
			cache.manage(texture, ObjectLoader::Pointer());
			return texture;
		}
	}
	else 
	{
		return existingObjects.front();
	}

	log::error("Unable to load texture from %s", fileName.c_str());
	return checkersTexture();
}

inline Texture::Pointer RenderInterface::checkersTexture()
{
	if (_checkersTexture.invalid())
	{
		const uint32_t colors[] = { 0xFFFF00FF, 0xFF00FF00 };
		uint32_t numColors = static_cast<uint32_t>(sizeof(colors) / sizeof(colors[0]));
		
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->size = vec2i(16);
		desc->format = TextureFormat::RGBA8;
		desc->data.resize(4 * static_cast<uint32_t>(desc->size.square()));
		
		uint32_t* data = reinterpret_cast<uint32_t*>(desc->data.data());
		for (uint32_t p = 0, e = static_cast<uint32_t>(desc->size.square()); p < e; ++p)
		{
			uint32_t ux = static_cast<uint32_t>(desc->size.x);
			uint32_t colorIndex = ((p / ux) + (p % ux)) % numColors;
			data[p] = colors[colorIndex];
		}

		_checkersTexture = createTexture(desc);
	}
	return _checkersTexture;
}

inline Texture::Pointer RenderInterface::whiteTexture()
{
	if (_whiteTexture.invalid())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->size = vec2i(4);
		desc->format = TextureFormat::RGBA8;
		desc->data.resize(4 * static_cast<uint32_t>(desc->size.square()));
		desc->data.fill(255);
		_whiteTexture = createTexture(desc);
	}
	return _whiteTexture;
}

inline Texture::Pointer RenderInterface::blackTexture()
{
	if (_blackTexture.invalid())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->size = vec2i(4);
		desc->format = TextureFormat::RGBA8;
		desc->data.resize(4 * static_cast<uint32_t>(desc->size.square()));
		desc->data.fill(0);
		_blackTexture = createTexture(desc);
	}
	return _blackTexture;
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

inline Sampler::Pointer RenderInterface::clampSampler()
{
	if (_clampSampler.invalid())
	{
		Sampler::Description desc;
		desc.wrapU = TextureWrap::ClampToEdge;
		desc.wrapV = TextureWrap::ClampToEdge;
		desc.wrapW = TextureWrap::ClampToEdge;
		_clampSampler = createSampler(desc);
	}
	return _clampSampler;
}

inline Sampler::Pointer RenderInterface::nearestSampler()
{
	if (_nearestSampler.invalid())
	{
		Sampler::Description desc;
		desc.magFilter = TextureFiltration::Nearest;
		desc.minFilter = TextureFiltration::Nearest;
		_nearestSampler = createSampler(desc);
	}
	return _nearestSampler;
}

inline void RenderInterface::initInternalStructures()
{
	_sharedConstantBuffer.init(this);
	_sharedMaterialLibrary.init(this);

	whiteTexture();
	blackTexture();
	checkersTexture();
	defaultSampler();
}

inline void RenderInterface::shutdownInternalStructures()
{
	_sharedMaterialLibrary.shutdown();
	_sharedConstantBuffer.shutdown();
	_defaultSampler.reset(nullptr);
	_checkersTexture.reset(nullptr);
	_whiteTexture.reset(nullptr);
	_blackTexture.reset(nullptr);
	_nearestSampler.reset(nullptr);
	_clampSampler.reset(nullptr);
}

inline Buffer::Pointer RenderInterface::createDataBuffer(const std::string& name, uint32_t size)
{
	Buffer::Description desc;
	desc.size = size;
	desc.location = Buffer::Location::Host;
	desc.usage = Buffer::Usage::Constant;
	return createBuffer(name, desc);
}

inline Buffer::Pointer RenderInterface::createDataBuffer(const std::string& name, const BinaryDataStorage& data)
{
	Buffer::Description desc;
	desc.size = data.size();
	desc.location = Buffer::Location::Host;
	desc.usage = Buffer::Usage::Constant;
	desc.initialData = BinaryDataStorage(data.data(), data.size());
	return createBuffer(name, desc);
}

inline Buffer::Pointer RenderInterface::createVertexBuffer(const std::string& name, const VertexStorage::Pointer& vs, Buffer::Location location)
{
	Buffer::Description desc;
	desc.size = vs->data().size();
	desc.location = location;
	desc.usage = Buffer::Usage::Vertex;
	desc.initialData = BinaryDataStorage(vs->data().data(), vs->data().size());
	return createBuffer(name, desc);
}

inline Buffer::Pointer RenderInterface::createIndexBuffer(const std::string& name, const IndexArray::Pointer& ia , Buffer::Location location)
{
	Buffer::Description desc;
	desc.size = ia->dataSize();
	desc.location = location;
	desc.usage = Buffer::Usage::Index;
	desc.initialData = BinaryDataStorage(ia->data(), ia->dataSize());
	return createBuffer(name, desc);
}

inline Texture::Pointer RenderInterface::generateHammersleySet(uint32_t size)
{
	auto inv = [](uint32_t bits) -> float
	{
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return static_cast<float>(static_cast<double>(bits) * 2.3283064365386963e-10);
	};

	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	desc->size = vec2i(size, 1);
	desc->format = TextureFormat::RG32F;
	desc->data.resize(size * sizeof(vec2));
	desc->data.fill(0);
	vec2* data = reinterpret_cast<vec2*>(desc->data.binary());
	for (uint32_t i = 0; i < size; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(size);
		*data++ = vec2(t, inv(i));
	}
	return createTexture(desc);
}

}
