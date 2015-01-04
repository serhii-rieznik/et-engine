/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/rendering/renderer.h>
#include <et/vertexbuffer/indexarray.h>

using namespace et;

extern const std::string fullscreen_vertex_shader; 
extern const std::string fullscreen_scaled_vertex_shader;
extern const std::string scaled_copy_vertex_shader;
extern const std::string copy_fragment_shader;
extern const std::string depth_fragment_shader;

Renderer::Renderer(RenderContext* rc) :
	_rc(rc), _defaultTextureBindingUnit(6)
{
#if !defined(ET_CONSOLE_APPLICATION)
	IndexArray::Pointer ib = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 4, PrimitiveType::TriangleStrips);
	
	ib->linearize(4);
	
	VertexArray::Pointer vb = VertexArray::Pointer::create(VertexDeclaration(false,
		VertexAttributeUsage::Position, VertexAttributeType::Vec2), 4);
	
	RawDataAcessor<vec2> pos = vb->chunk(VertexAttributeUsage::Position).accessData<vec2>(0);
	pos[0] = vec2(-1.0f, -1.0f);
	pos[1] = vec2( 1.0f, -1.0f);
	pos[2] = vec2(-1.0f,  1.0f);
	pos[3] = vec2( 1.0f,  1.0f);

	_fullscreenQuadVao = rc->vertexBufferFactory().createVertexArrayObject("__et__internal__fullscreen_vao__",
		vb, BufferDrawType::Static, ib, BufferDrawType::Static);

	_fullscreenProgram = rc->programFactory().genProgram("__et__fullscreeen__program__",
		fullscreen_vertex_shader, copy_fragment_shader);
	_fullscreenProgram->setUniform("color_texture", _defaultTextureBindingUnit);

	_fullscreenDepthProgram = rc->programFactory().genProgram("__et__fullscreeen__depth__program__",
		fullscreen_vertex_shader, depth_fragment_shader);
	_fullscreenDepthProgram->setUniform("depth_texture", _defaultTextureBindingUnit);
	_fullScreenDepthProgram_FactorUniform = _fullscreenDepthProgram->getUniform("factor");

	_fullscreenScaledProgram = rc->programFactory().genProgram("__et__fullscreeen_scaled_program__",
		fullscreen_scaled_vertex_shader, copy_fragment_shader);
	_fullscreenScaledProgram->setUniform("color_texture", _defaultTextureBindingUnit);
	_fullScreenScaledProgram_PSUniform = _fullscreenScaledProgram->getUniform("vScale");

	_scaledProgram = rc->programFactory().genProgram("__et__scaled_program__",
		scaled_copy_vertex_shader, copy_fragment_shader);
	_scaledProgram->setUniform("color_texture", _defaultTextureBindingUnit);
	_scaledProgram_PSUniform = _scaledProgram->getUniform("PositionScale");
#endif
}

void Renderer::clear(bool color, bool depth)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::fullscreenPass()
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindVertexArray(_fullscreenQuadVao);
	drawAllElements(_fullscreenQuadVao->indexBuffer());
#endif
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_fullscreenProgram);
	fullscreenPass();
#endif
}

void Renderer::renderFullscreenDepthTexture(const Texture::Pointer& texture, float factor)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_fullscreenDepthProgram);
	_fullscreenDepthProgram->setUniform(_fullScreenDepthProgram_FactorUniform, factor);
	fullscreenPass();
#endif
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture, const vec2& scale)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_fullscreenScaledProgram);
	_scaledProgram->setUniform(_fullScreenScaledProgram_PSUniform, scale);
	fullscreenPass();
#endif
}

void Renderer::renderTexture(const Texture::Pointer& texture, const vec2& position, const vec2& size)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_scaledProgram);
	_scaledProgram->setUniform(_scaledProgram_PSUniform, vec4(position, size));
	fullscreenPass();
#endif
}

vec2 Renderer::currentViewportCoordinatesToScene(const vec2i& coord)
{
	auto vpSize = _rc->renderState().viewportSizeFloat();
	return vec2(2.0f * static_cast<float>(coord.x) / vpSize.x - 1.0f,
		1.0f - 2.0f * static_cast<float>(coord.y) / vpSize.y );
}

vec2 Renderer::currentViewportSizeToScene(const vec2i& size)
{
	auto vpSize = _rc->renderState().viewportSizeFloat();
	return vec2(2.0f * static_cast<float>(size.x) / vpSize.x, 2.0f * static_cast<float>(size.y) / vpSize.y);
}

void Renderer::renderTexture(const Texture::Pointer& texture, const vec2i& position, const vec2i& size)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (texture.invalid()) return;
	
	vec2i sz;
	sz.x = (size.x == -1) ? texture->width() : size.x;
	sz.y = (size.y == -1) ? texture->height() : size.y;
	renderTexture(texture, currentViewportCoordinatesToScene(position + vec2i(0, sz.y)), currentViewportSizeToScene(sz));
#endif
}

void Renderer::drawElements(const IndexBuffer& ib, size_t first, size_t count)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::drawElementsInstanced(const IndexBuffer& ib, size_t first, size_t count, size_t instances)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::drawElements(PrimitiveType pt, const IndexBuffer& ib, size_t first, size_t count)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::drawAllElements(const IndexBuffer& ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::drawElementsBaseIndex(const VertexArrayObject& vao, int base, size_t first, size_t count)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Renderer::readFramebufferData(const vec2i& size, TextureFormat format, DataType dataType, BinaryDataStorage& data)
{
}

BinaryDataStorage Renderer::readFramebufferData(const vec2i& size, TextureFormat format, DataType dataType)
{
	BinaryDataStorage result(size.square() * bitsPerPixelForTextureFormat(format, dataType));
	readFramebufferData(size, format, dataType, result);
	return result;
}

/*
* Default shaders
*/

const std::string fullscreen_vertex_shader = "";
const std::string fullscreen_scaled_vertex_shader = "";
const std::string scaled_copy_vertex_shader = "";
const std::string copy_fragment_shader = "";
const std::string depth_fragment_shader = "";

namespace et
{
	VertexAttributeType openglTypeToVertexAttributeType(uint32_t value)
	{
		switch (value)
		{
		default:
			ET_FAIL_FMT("Unsupported OpenGL type %u (0x%X)", value, value);
		}
	}
}