/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/geometry.h>
#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/renderer.h>
#include <et/rendering/material.h>
#include <et/rendering/indexarray.h>
#include <et/rendering/vertexstorage.h>

using namespace et;

extern const std::string fullscreen_vertex_shader; 
extern const std::string fullscreen_scaled_vertex_shader;
extern const std::string scaled_copy_vertex_shader;
extern const std::string scaled_rotated_copy_vertex_shader;
extern const std::string copy_fragment_shader;
extern const std::string depth_fragment_shader;

Renderer::Renderer(RenderContext* rc) :
	_defaultTextureBindingUnit(7), _rc(rc)
{
	checkOpenGLError("Renderer::Renderer", 0);

	IndexArray::Pointer ib = IndexArray::Pointer::create(IndexArrayFormat::Format_8bit, 4, PrimitiveType::TriangleStrips);
	ib->linearize(4);
	
	auto vd = VertexDeclaration(false, VertexAttributeUsage::Position, DataType::Vec2);
	VertexStorage::Pointer vb = VertexStorage::Pointer::create(vd, 4);
	auto pos = vb->accessData<DataType::Vec2>(VertexAttributeUsage::Position, 0);
	pos[0] = vec2(-1.0f, -1.0f);
	pos[1] = vec2( 1.0f, -1.0f);
	pos[2] = vec2(-1.0f,  1.0f);
	pos[3] = vec2( 1.0f,  1.0f);

	_fullscreenQuadVao = rc->vertexBufferFactory().createVertexArrayObject("__et__internal__fullscreen_vao__",
		vb, BufferDrawType::Static, ib, BufferDrawType::Static);

#if (ET_OPENGLES)
	const std::string textureTypeDefines[TextureTarget_max] =
	{
		"#define TEXTURE_2D", // Texture_2D,
		"#define TEXTURE_2D", // Texture_2D_Array,
		"#define TEXTURE_2D", // Texture_Rectangle,
		"#define TEXTURE_CUBE", // Texture_Cube,
	};
#else
	const std::string textureTypeDefines[TextureTarget_max] =  
	{
		"#define TEXTURE_2D", // Texture_2D,
		"#define TEXTURE_2D_ARRAY", // Texture_2D_Array,
		"#define TEXTURE_RECTANGLE", // Texture_Rectangle,
		"#define TEXTURE_CUBE", // Texture_Cube,
	};
#endif
	StringList currentDefines(1);
	for (uint32_t i = 0; i < TextureTarget_max; ++i)
	{
		currentDefines[0] = textureTypeDefines[i];

		_fullscreenProgram[i] = rc->materialFactory().genProgram("__et__fullscreeen__program__", 
			fullscreen_vertex_shader, copy_fragment_shader, currentDefines);
		_fullscreenProgram[i]->setUniform("color_texture", _defaultTextureBindingUnit);
	}

	_fullscreenDepthProgram = rc->materialFactory().genProgram("__et__fullscreeen__depth__program__",
		fullscreen_vertex_shader, depth_fragment_shader);
	_fullscreenDepthProgram->setUniform("depth_texture", _defaultTextureBindingUnit);
	_fullScreenDepthProgram_FactorUniform = _fullscreenDepthProgram->getUniform("factor");

	_fullscreenScaledProgram = rc->materialFactory().genProgram("__et__fullscreeen_scaled_program__",
		fullscreen_scaled_vertex_shader, copy_fragment_shader);
	_fullscreenScaledProgram->setUniform("color_texture", _defaultTextureBindingUnit);
	_fullScreenScaledProgram_PSUniform = _fullscreenScaledProgram->getUniform("vScale");
	_fullScreenScaledProgram_TintUniform = _fullscreenScaledProgram->getUniform("tint");

	_scaledProgram = rc->materialFactory().genProgram("__et__scaled_program__",
		scaled_copy_vertex_shader, copy_fragment_shader);
	_scaledProgram->setUniform("color_texture", _defaultTextureBindingUnit);
	_scaledProgram_PSUniform = _scaledProgram->getUniform("PositionScale");
	_scaledProgram_TintUniform = _scaledProgram->getUniform("tint");

	_scaledRotatedProgram = rc->materialFactory().genProgram("__et__scaled_rotated_program__",
		scaled_rotated_copy_vertex_shader, copy_fragment_shader);
	_scaledRotatedProgram->setUniform("color_texture", _defaultTextureBindingUnit);
	_scaledRotatedProgram_PSUniform = _scaledRotatedProgram->getUniform("PositionScale");
	_scaledRotatedProgram_TintUniform = _scaledRotatedProgram->getUniform("tint");
	_scaledRotatedProgram_AngleUniform = _scaledRotatedProgram->getUniform("angle");
}

void Renderer::clear(bool color, bool depth)
{
	ET_ASSERT(!depth || (depth && _rc->renderState().depthState().depthWriteEnabled));
	ET_ASSERT(!color || (color && (_rc->renderState().colorMask() != static_cast<uint32_t>(ColorMask::None))));
	
	GLbitfield clearMask = (color * GL_COLOR_BUFFER_BIT) + (depth * GL_DEPTH_BUFFER_BIT);
	if (clearMask != 0)
	{
		glClear(clearMask);
	}
}

void Renderer::fullscreenPass()
{
	_rc->renderState().bindVertexArrayObject(_fullscreenQuadVao);
	drawAllElements(_fullscreenQuadVao->indexBuffer());
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture, const vec4& tint)
{
	auto prog = _fullscreenProgram[static_cast<int>(texture->target())];

	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(prog);
	prog->setUniform("color_texture_size", texture->sizeFloat());
	prog->setUniform("tint", tint);
	fullscreenPass();
}

void Renderer::renderFullscreenDepthTexture(const Texture::Pointer& texture, float factor)
{
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_fullscreenDepthProgram);
	_fullscreenDepthProgram->setUniform(_fullScreenDepthProgram_FactorUniform, factor);
	fullscreenPass();
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture, const vec2& scale, const vec4& tint)
{
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_fullscreenScaledProgram);
	_scaledProgram->setUniform(_fullScreenScaledProgram_PSUniform, scale);
	_scaledProgram->setUniform(_fullScreenScaledProgram_TintUniform, tint);
	fullscreenPass();
}

void Renderer::renderTexture(const Texture::Pointer& texture, const vec2& position, const vec2& size, const vec4& tint)
{
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_scaledProgram);
	_scaledProgram->setUniform(_scaledProgram_PSUniform, vec4(position, size));
	_scaledProgram->setUniform(_scaledProgram_TintUniform, tint);
	fullscreenPass();
}

void Renderer::renderTextureRotated(const Texture::Pointer& texture, float angle, const vec2& position,
	const vec2& size, const vec4& tint)
{
	_rc->renderState().bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState().bindProgram(_scaledRotatedProgram);
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_PSUniform, vec4(position, size));
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_TintUniform, tint);
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_AngleUniform, angle);
	fullscreenPass();
}

vec2 Renderer::currentViewportCoordinatesToScene(const vec2i& coord)
{
	auto vpSize = vector2ToFloat(_rc->renderState().viewportSize());
	return vec2(2.0f * static_cast<float>(coord.x) / vpSize.x - 1.0f,
		1.0f - 2.0f * static_cast<float>(coord.y) / vpSize.y );
}

vec2 Renderer::currentViewportSizeToScene(const vec2i& size)
{
	auto vpSize = vector2ToFloat(_rc->renderState().viewportSize());
	return vec2(2.0f * static_cast<float>(size.x) / vpSize.x, 2.0f * static_cast<float>(size.y) / vpSize.y);
}

void Renderer::renderTexture(const Texture::Pointer& texture, const vec2i& position, const vec2i& size, const vec4& tint)
{
	if (texture.invalid()) return;
	
	vec2i sz;
	sz.x = (size.x == -1) ? texture->width() : size.x;
	sz.y = (size.y == -1) ? texture->height() : size.y;
	renderTexture(texture, currentViewportCoordinatesToScene(position + vec2i(0, sz.y)), currentViewportSizeToScene(sz), tint);
}

void Renderer::renderTextureRotated(const Texture::Pointer& texture, float angle, const vec2i& position,
	const vec2i& size, const vec4& tint)
{
	if (texture.invalid()) return;
	
	vec2i sz;
	sz.x = (size.x == -1) ? texture->width() : size.x;
	sz.y = (size.y == -1) ? texture->height() : size.y;
	
	renderTextureRotated(texture, angle, currentViewportCoordinatesToScene(position + vec2i(0, sz.y)),
		currentViewportSizeToScene(sz), tint);
}

void Renderer::drawElements(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count)
{
	ET_ASSERT(ib.valid());
	
	etDrawElements(primitiveTypeValue(ib->primitiveType()), static_cast<GLsizei>(count),
		dataFormatValue(ib->dataFormat()), ib->indexOffset(first));
}

void Renderer::drawElementsInstanced(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count, uint32_t instances)
{
	ET_ASSERT(ib.valid());
	
	etDrawElementsInstanced(primitiveTypeValue(ib->primitiveType()), static_cast<GLsizei>(count),
		dataFormatValue(ib->dataFormat()), ib->indexOffset(first), static_cast<GLsizei>(instances));
}

void Renderer::drawElements(PrimitiveType pt, const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count)
{
	ET_ASSERT(ib.valid());
	
	etDrawElements(primitiveTypeValue(pt), static_cast<GLsizei>(count), dataFormatValue(ib->dataFormat()),
		ib->indexOffset(first));
}

void Renderer::drawAllElements(const IndexBuffer::Pointer& ib)
{
	ET_ASSERT(ib.valid());
	
	etDrawElements(primitiveTypeValue(ib->primitiveType()), static_cast<GLsizei>(ib->size()),
		dataFormatValue(ib->dataFormat()), nullptr);
}

void Renderer::drawElementsBaseIndex(const VertexArrayObject::Pointer& vao, int base, uint32_t first, uint32_t count)
{
	ET_ASSERT(vao->indexBuffer().valid());
	
	const IndexBuffer::Pointer& ib = vao->indexBuffer();

#if (ET_OPENGLES)
	
	ET_ASSERT(vao->vertexBuffer().valid());
	
	const VertexBuffer::Pointer& vb = vao->vertexBuffer();
	RenderState& rs = _rc->renderState();
	rs.bindVertexArrayObject(vao);
	rs.bindBuffer(vb);
	rs.setVertexAttributesBaseIndex(vb->declaration(), base);
	
	etDrawElements(primitiveTypeValue(ib->primitiveType()), static_cast<GLsizei>(count),
		dataFormatValue(ib->dataFormat()), ib->indexOffset(first));
	
#else
	
	etDrawElementsBaseVertex(primitiveTypeValue(ib->primitiveType()), static_cast<GLsizei>(count),
		dataFormatValue(ib->dataFormat()), ib->indexOffset(first), base);
	
#endif
}

void Renderer::drawElementsSequentially(PrimitiveType primitiveType, uint32_t first, uint32_t count)
{
	auto pvalue = primitiveTypeValue(primitiveType);
	glDrawArrays(pvalue, GLint(first), GLsizei(count));
	checkOpenGLError("glDrawArrays(%u, %u, %u)", static_cast<uint32_t>(pvalue),
		static_cast<uint32_t>(first), static_cast<uint32_t>(count));
}

void Renderer::readFramebufferData(const vec2i& size, TextureFormat format, DataFormat dataType, BinaryDataStorage& data)
{
	data.fitToSize(size.square() * bitsPerPixelForTextureFormat(format, dataType));
	glReadPixels(0, 0, size.x, size.y, textureFormatValue(format), dataFormatValue(dataType), data.data());
	checkOpenGLError("glReadPixels");
}

BinaryDataStorage Renderer::readFramebufferData(const vec2i& size, TextureFormat format, DataFormat dataType)
{
	BinaryDataStorage result(size.square() * bitsPerPixelForTextureFormat(format, dataType));
	readFramebufferData(size, format, dataType, result);
	return result;
}

void Renderer::finishRendering()
{
	glFinish();
}

/*
 * Default shaders
 */

const std::string fullscreen_vertex_shader = R"(
etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;
void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(Vertex, 0.0, 1.0);
})";

const std::string fullscreen_scaled_vertex_shader = R"(
uniform vec2 vScale;
etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;
void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(vScale * Vertex, 0.0, 1.0);
})";

const std::string scaled_copy_vertex_shader = R"(
uniform vec4 PositionScale;
etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;
void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(PositionScale.xy + TexCoord * PositionScale.zw, 0.0, 1.0);
})";

const std::string scaled_rotated_copy_vertex_shader = R"(
uniform vec4 PositionScale;
uniform float angle;
etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;
void main()
{
	float ca = cos(angle);
	float sa = sin(angle);
	float rotatedX = ca * Vertex.x - sa * Vertex.y;
	float rotatedY = sa * Vertex.x + ca * Vertex.y;
	TexCoord = 0.5 * vec2(rotatedX, rotatedY) + vec2(0.5);
	gl_Position = vec4(PositionScale.xy + (0.5 + 0.5 * Vertex) * PositionScale.zw, 0.0, 1.0);
})";

const std::string copy_fragment_shader = R"(
#if defined(TEXTURE_CUBE)
	uniform etLowp samplerCube color_texture;
#elif defined(TEXTURE_RECTANGLE)
	uniform etLowp sampler2DRect color_texture;
#elif defined(TEXTURE_2D_ARRAY)
	uniform etLowp sampler2DArray color_texture;
#else
	uniform etLowp sampler2D color_texture;
#endif

uniform etHighp vec2 color_texture_size;
uniform etLowp vec4 tint;
etFragmentIn etHighp vec2 TexCoord;

void main()
{
#if defined(TEXTURE_CUBE)

	etFragmentOut = tint * etTextureCube(color_texture, vec3(TexCoord, 0.0));

#elif defined(TEXTURE_RECTANGLE)

	etFragmentOut = tint * etTextureRect(color_texture, TexCoord * color_texture_size);

#elif defined(TEXTURE_2D_ARRAY)

	etFragmentOut = tint * etTexture2DArray(color_texture, vec3(TexCoord, 0.0));

#else

	etFragmentOut = tint * etTexture2D(color_texture, TexCoord);

#endif
})";

const std::string depth_fragment_shader = R"(
uniform etHighp sampler2D depth_texture;
uniform etHighp float factor;
etFragmentIn etHighp vec2 TexCoord;
void main()
{
	etFragmentOut = pow(etTexture2D(depth_texture, TexCoord), vec4(factor));
})";
