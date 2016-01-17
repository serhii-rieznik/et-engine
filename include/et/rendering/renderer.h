/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/renderstate.h>
#include <et/rendering/renderbatch.h>

namespace et
{
	class RenderContext;
	class Renderer : public Shared
	{
	public:
		ET_DECLARE_POINTER(Renderer)
		
	public: 
		Renderer(RenderContext*);

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
		void drawElementsBaseIndex(const VertexArrayObject::Pointer& vao, int base, uint32_t first, uint32_t count);
		void drawElementsSequentially(PrimitiveType, uint32_t first, uint32_t count);
		
		BinaryDataStorage readFramebufferData(const vec2i&, TextureFormat, DataFormat);
		void readFramebufferData(const vec2i&, TextureFormat, DataFormat, BinaryDataStorage&);

		vec2 currentViewportCoordinatesToScene(const vec2i& coord);
		vec2 currentViewportSizeToScene(const vec2i& size);
		
		void finishRendering();
		
		/*
		 * Render batches stuff
		 */
		void setCurrentCamera(Camera*);
		void pushRenderBatch(RenderBatch::Pointer);
		
		Camera* currentCamera() const
			{ return _currentCamera; }

		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(uint32_t, defaultTextureBindingUnit, setDefaultTextureBindingUnit)
		
	private:
		Renderer& operator = (const Renderer&)
			{ return *this; }

	private:
		RenderContext* _rc = nullptr;
		
		Camera _defaultCamera;
		Camera* _currentCamera = nullptr;
		
		ObjectsCache _sharedCache;
		VertexArrayObject::Pointer _fullscreenQuadVao;
		
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
}
