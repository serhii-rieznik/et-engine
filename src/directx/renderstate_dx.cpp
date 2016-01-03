/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/renderstate.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

#include <et/vertexbuffer/vertexdeclaration.h>
#include <et/rendering/rendercontext.h>

using namespace et;

RenderState::State::State()
{
	enabledVertexAttributes.fill(0);
	drawBuffers.fill(0);
}

PreservedRenderStateScope::PreservedRenderStateScope(RenderContext* rc, bool shouldApplyBefore) :
	_rc(rc), _state(RenderState::currentState())
{
	if (shouldApplyBefore)
		_rc->renderState().applyState(_state);
}

PreservedRenderStateScope::~PreservedRenderStateScope()
{
	_rc->renderState().applyState(_state);
}

void RenderState::setRenderContext(RenderContext* rc)
{
	_rc = rc;
	
#if !defined(ET_CONSOLE_APPLICATION)
	_currentState = RenderState::currentState();
#endif
}

void RenderState::setMainViewportSize(const vec2i& sz, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setViewportSize(const vec2i& sz, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setActiveTextureUnit(uint32_t unit, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindTexture(uint32_t unit, uint32_t texture, TextureTarget target, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindProgram(uint32_t program, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindProgram(const Program::Pointer& prog, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindBuffer(uint32_t target, uint32_t buffer, bool force)
{ 
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setVertexAttributes(const VertexDeclaration& decl, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	for (uint32_t i = 0; i < VertexAttributeUsage_max; ++i)
		setVertexAttribEnabled(i, decl.has(static_cast<VertexAttributeUsage>(i)), force);
	
	setVertexAttributesBaseIndex(decl, 0);
#endif
}

void RenderState::setVertexAttributesBaseIndex(const VertexDeclaration& decl, size_t index, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindBuffer(const VertexBuffer::Pointer& buffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindBuffer(const IndexBuffer& buf, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindBuffers(const VertexBuffer::Pointer& vb, const IndexBuffer& ib, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindVertexArray(uint32_t buffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindVertexArray(const VertexArrayObject& vao, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::resetBufferBindings()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindTexture(uint32_t unit, const Texture::Pointer& texture, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindFramebuffer(uint32_t framebuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindFramebuffer(uint32_t framebuffer, uint32_t target, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindReadFramebuffer(uint32_t framebuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindDrawFramebuffer(uint32_t framebuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindFramebuffer(const Framebuffer::Pointer& fbo, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindRenderbuffer(uint32_t renderbuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setDefaultFramebuffer(const Framebuffer::Pointer& framebuffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::bindDefaultFramebuffer(bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setDrawBuffersCount(int32_t count)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setDepthMask(bool enable, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setDepthTest(bool enable, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setDepthFunc(DepthFunc func, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setBlend(bool enable, BlendState blend, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::vertexArrayDeleted(uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::vertexBufferDeleted(uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::indexBufferDeleted(uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::programDeleted(uint32_t program)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::textureDeleted(uint32_t texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::frameBufferDeleted(uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setVertexAttribEnabled(uint32_t attrib, bool enabled, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setVertexAttribPointer(const VertexElement& e, size_t baseIndex, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setCulling(bool enabled, CullState cull, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setPolygonOffsetFill(bool enabled, float factor, float units, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setWireframeRendering(bool wire, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setClearColor(const vec4& color, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setColorMask(ColorMask mask, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setColorMask(size_t mask, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setClearDepth(float depth, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setClip(bool enable, const recti& clip, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::setSampleAlphaToCoverage(bool enable, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::reset()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void RenderState::applyState(const RenderState::State& s)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

RenderState::State RenderState::currentState()
{
	RenderState::State result;

#if !defined(ET_CONSOLE_APPLICATION)
#endif

	return result;
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
