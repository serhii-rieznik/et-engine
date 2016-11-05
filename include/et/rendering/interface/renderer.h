/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/context.h>
#include <et/rendering/constbuffer.h>
#include <et/rendering/sharedvariables.h>
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

	SharedVariables& sharedVariables()
		{ return _sharedVariables; }

	ConstBuffer& sharedConstBuffer()
		{ return _sharedConstBuffer; }

	MaterialLibrary& sharedMaterialLibrary()
		{ return _sharedMaterialLibrary; }

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
	virtual Program::Pointer createProgram(const std::string& vs, const std::string& fs) = 0;

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
	SharedVariables _sharedVariables;
	ConstBuffer _sharedConstBuffer;
	MaterialLibrary _sharedMaterialLibrary;
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
	_sharedVariables.init(this);
	_sharedConstBuffer.init(this);
	_sharedMaterialLibrary.init(this);
}

inline void RenderInterface::shutdownInternalStructures()
{
	_sharedMaterialLibrary.shutdown();
	_sharedConstBuffer.shutdown();
	_sharedVariables.shutdown();
}


	/*
	 class RenderContext;
	class Renderer : public RendererInterface
	{
	public:
		Renderer(RenderContext*);

	private:
		void clear(bool color = true, bool depth = true);

		void fullscreenPass();
		
		void renderFullscreenTexture(const Texture::Pointer&, const vec4& = vec4(1.0f));
		void renderFullscreenTexture(const Texture::Pointer&, const vec2& scale, const vec4& = vec4(1.0f));

		void renderFullscreenDepthTexture(const Texture::Pointer&, float factor);

		void renderTexture(const Texture::Pointer&, const vec2& position, const vec2& size,
			const vec4& = vec4(1.0f));
		void renderTextureRotated(const Texture::Pointer&, float, const vec2& position, const vec2& size,
			const vec4& = vec4(1.0f));
		
		void renderTexture(const Texture::Pointer&, const vec2i& position, const vec2i& size = vec2i(-1),
			const vec4& = vec4(1.0f));
		void renderTextureRotated(const Texture::Pointer&, float angle, const vec2i& position,
			const vec2i& size = vec2i(-1), const vec4& = vec4(1.0f));

		void drawElements(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count);
		void drawElements(PrimitiveType primitiveType, const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count);
		void drawAllElements(const IndexBuffer::Pointer& ib);

		void drawElementsInstanced(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count, uint32_t instances);
		void drawElementsBaseIndex(const VertexStream::Pointer& vao, int base, uint32_t first, uint32_t count);
		void drawElementsSequentially(PrimitiveType, uint32_t first, uint32_t count);
		
		BinaryDataStorage readFramebufferData(const vec2i&, TextureFormat, DataFormat);
		void readFramebufferData(const vec2i&, TextureFormat, DataFormat, BinaryDataStorage&);

		vec2 currentViewportCoordinatesToScene(const vec2i& coord);
		vec2 currentViewportSizeToScene(const vec2i& size);
		
		void finishRendering();

		uint32_t defaultTextureBindingUnit() const
			{ return _defaultTextureBindingUnit; }

		void setDefaultTextureBindingUnit(uint32_t defaultTextureBindingUnit)
			{ _defaultTextureBindingUnit = defaultTextureBindingUnit; }

	private:
		Renderer(Renderer&&) = delete;
		Renderer(const Renderer&) = delete;
		Renderer& operator = (const Renderer&) = delete;

	private:
		RenderContext* _rc = nullptr;
		uint32_t _defaultTextureBindingUnit = 7;
		
		ObjectsCache _sharedCache;
		VertexStream::Pointer _fullscreenQuadVao;
		
		Program::Pointer _fullscreenProgram[TextureTarget_max];
		Program::Pointer _fullscreenDepthProgram;
		Program::Pointer _fullscreenScaledProgram;
		Program::Pointer _scaledProgram;
		Program::Pointer _scaledRotatedProgram;
		
		Program::Uniform _scaledProgram_PSUniform;
		Program::Uniform _scaledProgram_TintUniform;
		Program::Uniform _scaledRotatedProgram_PSUniform;
		Program::Uniform _scaledRotatedProgram_TintUniform;
		Program::Uniform _scaledRotatedProgram_AngleUniform;
		Program::Uniform _fullScreenScaledProgram_PSUniform;
		Program::Uniform _fullScreenScaledProgram_TintUniform;
		Program::Uniform _fullScreenDepthProgram_FactorUniform;
	};
*/
}
