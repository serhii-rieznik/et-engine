/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/vertexbuffer/vertexdeclaration.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/renderstate.h>

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
		
	unsigned char blackColor[] = { 0, 0, 0, 255 };
	bindTexture(0, 0, TextureTarget::Texture_2D, true);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	etTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &blackColor);
	
	auto currentUnit = _currentState.boundTextures[TextureTarget::Texture_2D][_currentState.activeTextureUnit];
	bindTexture(_currentState.activeTextureUnit, currentUnit, TextureTarget::Texture_2D);
#endif
}

void RenderState::setMainViewportSize(const vec2i& sz, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (!force && (sz.x == _currentState.mainViewportSize.x) && (sz.y == _currentState.mainViewportSize.y)) return;

	_currentState.mainViewportSize = sz;
	_currentState.mainViewportSizeFloat = vec2(static_cast<float>(sz.x), static_cast<float>(sz.y));

	bool shouldSetViewport = (_currentState.boundFramebuffer == 0) ||
		(_defaultFramebuffer.valid() && (_currentState.boundFramebuffer == _defaultFramebuffer->apiHandle()));
	
	if (shouldSetViewport)
		etViewport(0, 0, _currentState.mainViewportSize.x, _currentState.mainViewportSize.y);
#endif
}

void RenderState::setViewportSize(const vec2i& sz, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (!force && (sz.x == _currentState.viewportSize.x) && (sz.y == _currentState.viewportSize.y)) return;

	_currentState.viewportSize = sz;
	_currentState.viewportSizeFloat = vec2(static_cast<float>(sz.x), static_cast<float>(sz.y));
	etViewport(0, 0, _currentState.viewportSize.x, _currentState.viewportSize.y);
#endif
}

void RenderState::setActiveTextureUnit(uint32_t unit, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if ((unit != _currentState.activeTextureUnit) || force)
	{
		_currentState.activeTextureUnit = unit;
		
		glActiveTexture(GL_TEXTURE0 + _currentState.activeTextureUnit);
		checkOpenGLError("glActiveTexture(GL_TEXTURE0 + %u)", _currentState.activeTextureUnit);
	}
#endif
}

void RenderState::bindTexture(uint32_t unit, uint32_t texture, TextureTarget target, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	setActiveTextureUnit(unit, force);
	
	if (force || (_currentState.boundTextures[target][unit] != texture))
	{
		_currentState.boundTextures[target][unit] = texture;
		etBindTexture(textureTargetValue(target), texture);
	}
#endif
}

void RenderState::bindProgram(uint32_t program, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (program != _currentState.boundProgram))
	{ 
		_currentState.boundProgram = program;
		etUseProgram(program);
	}
#endif
}

void RenderState::bindProgram(const Program::Pointer& prog, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(prog.valid());
	bindProgram(static_cast<uint32_t>(prog->apiHandle()), force);
#endif
}

void RenderState::bindBuffer(uint32_t target, uint32_t buffer, bool force)
{ 
#if !defined(ET_CONSOLE_APPLICATION)
	if ((target == GL_ARRAY_BUFFER) && (force || (_currentState.boundArrayBuffer != buffer)))
	{
		_currentState.boundArrayBuffer = buffer;
		etBindBuffer(target, buffer);
	}
	else if ((target == GL_ELEMENT_ARRAY_BUFFER) && (force || (_currentState.boundElementArrayBuffer != buffer)))
	{ 
		_currentState.boundElementArrayBuffer = buffer;
		etBindBuffer(target, buffer);
	}
	else if ((target != GL_ARRAY_BUFFER) && (target != GL_ELEMENT_ARRAY_BUFFER))
	{
		log::warning("Trying to bind buffer %u to unknown target %u", buffer, target);
	}
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
	for (size_t i = 0; i < decl.numElements(); ++i)
	{
		const VertexElement& e = decl.element(i);
		size_t dataOffset = index * (decl.interleaved() ? decl.dataSize() : vertexAttributeTypeSize(e.type()));
		setVertexAttribPointer(e, dataOffset, force);
	}
#endif
}

void RenderState::bindBuffer(const VertexBuffer& buffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (buffer.valid())
	{
		bindBuffer(GL_ARRAY_BUFFER, static_cast<uint32_t>(buffer->apiHandle()), force);
		setVertexAttributes(buffer->declaration(), force);
	}
	else
	{
		bindBuffer(GL_ARRAY_BUFFER, 0, force);
	}
#endif
}

void RenderState::bindBuffer(const IndexBuffer& buf, bool force)
{
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.valid() ? static_cast<uint32_t>(buf->apiHandle()) : 0, force);
}

void RenderState::bindBuffers(const VertexBuffer& vb, const IndexBuffer& ib, bool force)
{
	bindBuffer(vb, force);
	bindBuffer(ib, force);
}

void RenderState::bindVertexArray(uint32_t buffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects));
	
	if (force || (_currentState.boundVertexArrayObject != buffer))
	{
		_currentState.boundVertexArrayObject = buffer;
		etBindVertexArray(buffer);
	}
#endif
}

void RenderState::bindVertexArray(const VertexArrayObject& vao, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		bindVertexArray(vao.valid() ? static_cast<uint32_t>(vao->apiHandle()) : 0, force);
	}
	else
	{
		if (vao.valid())
			bindBuffers(vao->vertexBuffer(), vao->indexBuffer(), force);
		else
			bindBuffers(VertexBuffer(), IndexBuffer(), force);
	}
#endif
}

void RenderState::resetBufferBindings()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		bindVertexArray(0);
	
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	bindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void RenderState::bindTexture(uint32_t unit, const Texture::Pointer& texture, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (texture.valid())
		bindTexture(unit, static_cast<uint32_t>(texture->apiHandle()), texture->target(), force);
	else
		bindTexture(unit, 0, TextureTarget::Texture_2D, force);
#endif
}

void RenderState::bindFramebuffer(uint32_t framebuffer, bool force)
{
	bindFramebuffer(framebuffer, GL_FRAMEBUFFER, force);
}

void RenderState::bindFramebuffer(uint32_t framebuffer, uint32_t target, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.boundFramebuffer != framebuffer) ||
		(_currentState.boundDrawFramebuffer != framebuffer) ||
		(_currentState.boundReadFramebuffer != framebuffer))
	{
		_currentState.boundDrawFramebuffer = framebuffer;
		_currentState.boundReadFramebuffer = framebuffer;
		_currentState.boundFramebuffer = framebuffer;
		etBindFramebuffer(target, framebuffer);
	}
#endif
}

void RenderState::bindReadFramebuffer(uint32_t framebuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	bool alreadyBound = (_currentState.boundReadFramebuffer == framebuffer) ||
		(_currentState.boundFramebuffer == framebuffer);
	
	if (force || !alreadyBound)
	{
		_currentState.boundReadFramebuffer = framebuffer;
		etBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	}
#endif
}

void RenderState::bindDrawFramebuffer(uint32_t framebuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	bool alreadyBound = (_currentState.boundDrawFramebuffer == framebuffer) ||
		(_currentState.boundFramebuffer == framebuffer);
	
	if (force || !alreadyBound)
	{
		_currentState.boundDrawFramebuffer = framebuffer;
		etBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	}
#endif
}

void RenderState::bindFramebuffer(const Framebuffer::Pointer& fbo, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (fbo.valid())
	{
		bindFramebuffer(static_cast<uint32_t>(fbo->apiHandle()), GL_FRAMEBUFFER, force);
		
		setViewportSize(fbo->size(), force);
		
		if (fbo->hasRenderTargets())
			setDrawBuffersCount(fbo->drawBuffersCount());
	}
	else 
	{
		bindFramebuffer(0, GL_FRAMEBUFFER, force);
		setViewportSize(_currentState.mainViewportSize, force);
	}
#endif
}

void RenderState::bindRenderbuffer(uint32_t renderbuffer, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.boundRenderbuffer != renderbuffer))
	{
		_currentState.boundRenderbuffer = renderbuffer;
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		checkOpenGLError("glBindRenderbuffer");
	}
#endif
}

void RenderState::setDefaultFramebuffer(const Framebuffer::Pointer& framebuffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_defaultFramebuffer = framebuffer;

	if (_defaultFramebuffer.valid())
		setMainViewportSize(_defaultFramebuffer->size());
#endif
}

void RenderState::bindDefaultFramebuffer(bool force)
{
	bindFramebuffer(_defaultFramebuffer, force);
//	setDrawBuffersCount(1);
}

void RenderState::setDrawBuffersCount(int32_t count)
{
#if !defined(ET_CONSOLE_APPLICATION)
	
#	if (GL_VERSION_2_0)
		glDrawBuffers(count, drawBufferTargets());
		checkOpenGLError("glDrawBuffers(%d, ...)", count);
#	endif
	
#endif
}

void RenderState::setDepthMask(bool enable, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.depthMask != enable))
	{
		_currentState.depthMask = enable;
		glDepthMask(enable);
	}
#endif
}

void RenderState::setDepthTest(bool enable, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (enable != _currentState.depthTestEnabled))
	{
		_currentState.depthTestEnabled = enable;
		(enable ? glEnable : glDisable)(GL_DEPTH_TEST);
	}
#endif
}

void RenderState::setDepthFunc(DepthFunc func, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (func != _currentState.lastDepthFunc))
	{
		_currentState.lastDepthFunc = func;
		
		switch (_currentState.lastDepthFunc)
		{
			case DepthFunc::Less:
			{
				glDepthFunc(GL_LESS);
				break;
			}
				
			case DepthFunc::LessOrEqual:
			{
				glDepthFunc(GL_LEQUAL);
				break;
			}
				
			case DepthFunc::Equal:
			{
				glDepthFunc(GL_EQUAL);
				break;
			}
				
			case DepthFunc::GreaterOrEqual:
			{
				glDepthFunc(GL_GEQUAL);
				break;
			}
				
			case DepthFunc::Greater:
			{
				glDepthFunc(GL_GREATER);
				break;
			}
				
			case DepthFunc::Always:
			{
				glDepthFunc(GL_ALWAYS);
				break;
			}
				
			default:
				ET_FAIL("Invalid DepthFunc value");
		}
	}
#endif
}

void RenderState::setSeparateBlend(bool enable, BlendState color, BlendState alpha, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.blendEnabled != enable))
	{
		_currentState.blendEnabled = enable;
		(enable ? glEnable : glDisable)(GL_BLEND);
	}
	
	bool shouldSet = force;
	
	if (color != BlendState::Current)
	{
		shouldSet = true;
		_currentState.lastColorBlend = color;
	}

	if (alpha != BlendState::Current)
	{
		shouldSet = true;
		_currentState.lastAlphaBlend = alpha;
	}
	
	if (shouldSet)
	{
		std::pair<uint32_t, uint32_t> colorBlend = blendStateValue(_currentState.lastColorBlend);
		std::pair<uint32_t, uint32_t> alphaBlend = blendStateValue(_currentState.lastAlphaBlend);
		glBlendFuncSeparate(colorBlend.first, colorBlend.second, alphaBlend.first, alphaBlend.second);
		checkOpenGLError("glBlendFuncSeparate(%u, %u, %u, %u)", colorBlend.first, colorBlend.second,
			alphaBlend.first, alphaBlend.second);
	}
#endif
}

void RenderState::setBlend(bool enable, BlendState blend, bool force)
{
	setSeparateBlend(enable, blend, blend, force);
}

void RenderState::vertexArrayDeleted(uint32_t buffer)
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		ET_ASSERT(_currentState.boundVertexArrayObject == buffer)
		(void)buffer;
		
		bindBuffer(GL_ARRAY_BUFFER, 0, true);
		bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0, true);
		bindVertexArray(0);
	}
}

void RenderState::vertexBufferDeleted(uint32_t buffer)
{
	if (_currentState.boundArrayBuffer == buffer)
		bindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderState::indexBufferDeleted(uint32_t buffer)
{
	if (_currentState.boundElementArrayBuffer == buffer)
		bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RenderState::programDeleted(uint32_t program)
{
	if (_currentState.boundProgram == program)
		bindProgram(0, true);
}

void RenderState::textureDeleted(uint32_t texture)
{
	for (auto& target : _currentState.boundTextures)
	{
		for (auto& unit : target.second)
		{
			if (unit.second == texture)
				bindTexture(unit.first, unit.second, target.first);
		}
	}
}

void RenderState::frameBufferDeleted(uint32_t buffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_defaultFramebuffer.valid() && (_defaultFramebuffer->apiHandle() == buffer))
		_defaultFramebuffer = Framebuffer::Pointer();
	
	if (_currentState.boundFramebuffer == buffer)
		bindDefaultFramebuffer();
#endif
}

void RenderState::setVertexAttribEnabled(uint32_t attrib, bool enabled, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	bool wasEnabled = _currentState.enabledVertexAttributes[attrib] > 0;

	if (enabled && (!wasEnabled || force))
	{
		glEnableVertexAttribArray(attrib);
		checkOpenGLError("glEnableVertexAttribArray");
	}
	else if (!enabled && (wasEnabled || force))
	{
		glDisableVertexAttribArray(attrib);
		checkOpenGLError("glDisableVertexAttribArray");
	}
	
	_currentState.enabledVertexAttributes[attrib] = enabled;
#endif
}

void RenderState::setVertexAttribPointer(const VertexElement& e, size_t baseIndex, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	(void)force;
	
	glVertexAttribPointer(GLuint(e.usage()), static_cast<GLint>(e.components()), dataTypeValue(e.dataType()), false,
		e.stride(), reinterpret_cast<GLvoid*>(e.offset() + baseIndex));

	checkOpenGLError("glVertexAttribPointer");
#endif
}

void RenderState::setCulling(bool enabled, CullState cull, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.cullEnabled != enabled))
	{
		_currentState.cullEnabled = enabled;
		if (_currentState.cullEnabled)
		{
			glEnable(GL_CULL_FACE);
			checkOpenGLError("glEnable(GL_CULL_FACE)");
		}
		else
		{
			glDisable(GL_CULL_FACE);
			checkOpenGLError("glDisable(GL_CULL_FACE)");
		}
	}
	
	if ((cull != CullState::Current) && (force || (_currentState.lastCull != cull)))
	{
		_currentState.lastCull = cull;
		glCullFace(cull == CullState::Back ? GL_BACK : GL_FRONT);
		checkOpenGLError(cull == CullState::Back ? "glCullFace(GL_BACK)" : "glCullFace(GL_FRONT)");
	}
#endif
}

void RenderState::setPolygonOffsetFill(bool enabled, float factor, float units, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.polygonOffsetFillEnabled != enabled))
	{
		_currentState.polygonOffsetFillEnabled = enabled;
		(enabled ? glEnable : glDisable)(GL_POLYGON_OFFSET_FILL);
	}

	_currentState.polygonOffsetFactor = factor;
	_currentState.polygonOffsetUnits = units;
	glPolygonOffset(factor, units);
#endif
}

void RenderState::setWireframeRendering(bool wire, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.wireframe != wire))
	{
#		if (!ET_OPENGLES)
		_currentState.wireframe = wire;
		glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
#		endif
	}
#endif
}

void RenderState::setClearColor(const vec4& color, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || ((_currentState.clearColor - color).dotSelf() > std::numeric_limits<float>::epsilon()))
	{
		_currentState.clearColor = color;
		glClearColor(color.x, color.y, color.z, color.w);
		checkOpenGLError("RenderState::glClearColor");
	}
#endif
}

void RenderState::setColorMask(ColorMask mask, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	setColorMask(static_cast<size_t>(mask), force);
#endif
}

void RenderState::setColorMask(size_t mask, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.colorMask != mask))
	{
		_currentState.colorMask = mask;
		
		GLboolean redEnabled = (mask & static_cast<size_t>(ColorMask::Red)) != 0;
		GLboolean greenEnabled = (mask & static_cast<size_t>(ColorMask::Green)) != 0;
		GLboolean blueEnabled = (mask & static_cast<size_t>(ColorMask::Blue)) != 0;
		GLboolean alphaEnabled = (mask & static_cast<size_t>(ColorMask::Alpha)) != 0;
		glColorMask(redEnabled, greenEnabled, blueEnabled, alphaEnabled);
		checkOpenGLError("RenderState::setColorMask");
	}
#endif
}

void RenderState::setClearDepth(float depth, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (force || (_currentState.clearDepth != depth))
	{
		_currentState.clearDepth = depth;
		glClearDepth(depth);
		checkOpenGLError("RenderState::setClearDepth");
	}
#endif
}

void RenderState::setClip(bool enable, const recti& clip, bool force)
{
#if !defined(ET_CONSOLE_APPLICATION)
	
	if (force || (enable != _currentState.clipEnabled))
	{
		_currentState.clipEnabled = enable;
		(_currentState.clipEnabled ? glEnable : glDisable)(GL_SCISSOR_TEST);
		checkOpenGLError("RenderState::setClip");
	}

	if (force || (clip != _currentState.clipRect))
	{
		_currentState.clipRect = clip;
		glScissor(clip.left, clip.top, etMax(0, clip.width), etMax(0, clip.height));
		checkOpenGLError("RenderState::setClip - glScissor");
	}
	
#endif
}

void RenderState::setSampleAlphaToCoverage(bool enable, bool force)
{
	if (force || (_currentState.alphaToCoverage != enable))
	{
		_currentState.alphaToCoverage = enable;
		(enable ? glEnable : glDisable)(GL_SAMPLE_ALPHA_TO_COVERAGE);
		checkOpenGLError("setSampleAlphaToCoverage(%s)", enable ? "true" : "false");
	}
}

void RenderState::reset()
{
	applyState(RenderState::State());
}

void RenderState::applyState(const RenderState::State& s)
{
#if !defined(ET_CONSOLE_APPLICATION)
	setClearColor(s.clearColor, true);
	setColorMask(s.colorMask, true);
	setSeparateBlend(s.blendEnabled, s.lastColorBlend, s.lastAlphaBlend, true);
	setDepthFunc(s.lastDepthFunc, true);
	setDepthMask(s.depthMask, true);
	setDepthTest(s.depthTestEnabled, true);
	setPolygonOffsetFill(s.polygonOffsetFillEnabled, s.polygonOffsetFactor, s.polygonOffsetUnits, true);
	setWireframeRendering(s.wireframe, true);
	setCulling(s.cullEnabled, s.lastCull, true);
	setViewportSize(s.viewportSize, true);
	bindFramebuffer(s.boundFramebuffer, true);
	bindProgram(s.boundProgram, true);
	bindVertexArray(s.boundVertexArrayObject, true);
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, s.boundElementArrayBuffer, true);
	bindBuffer(GL_ARRAY_BUFFER, s.boundArrayBuffer, true);
	
	for (auto& target : s.boundTextures)
	{
		for (auto& unit : target.second)
			bindTexture(unit.first, unit.second, target.first, true);
	}
	
	for (uint32_t i = 0, e = static_cast<uint32_t>(VertexAttributeUsage::max); i < e; ++i)
		setVertexAttribEnabled(i, (s.enabledVertexAttributes[i] != 0), true);
	
	setActiveTextureUnit(s.activeTextureUnit, true);
#endif
}

RenderState::State RenderState::currentState()
{
	State s;
	
#if !defined(ET_CONSOLE_APPLICATION)
	checkOpenGLError("currentState()");
	
	int value = 0;
	
#	if defined(GL3_PROTOTYPES)
		int maxDrawBuffers = 0;
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
		checkOpenGLError("");
		
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			glGetIntegerv(GL_DRAW_BUFFER0 + i, &value);
			s.drawBuffers[i] = value;
			checkOpenGLError("");
		}
#	endif
	
	value = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &value);
	s.activeTextureUnit = static_cast<uint32_t>(value - GL_TEXTURE0);
	checkOpenGLError("");
	
	int maxTextureUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
	
	for (int i = 0; i < maxTextureUnits; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		
		value = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
		s.boundTextures[TextureTarget::Texture_2D][i] = value;
		
		value = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
		s.boundTextures[TextureTarget::Texture_Cube][i] = value;
	}
	
	glActiveTexture(GL_TEXTURE0 + s.activeTextureUnit);
	checkOpenGLError("");

	value = 0;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &value);
	s.boundArrayBuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &value);
	s.boundElementArrayBuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);
	s.boundFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);
	s.boundReadFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);
	s.boundDrawFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &value);
	s.boundRenderbuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &value);
	s.boundVertexArrayObject = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	bool shouldLoadVertexAttribs = (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x) ||
		(s.boundVertexArrayObject != 0);
	
	for (uint32_t i = 0, e = static_cast<uint32_t>(VertexAttributeUsage::max); shouldLoadVertexAttribs && (i < e); ++i)
	{
		int enabled = 0;
		glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
		s.enabledVertexAttributes[i] = (enabled > 0);
		checkOpenGLError("glGetVertexAttribiv");
	}
	
	value = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &value);
	s.boundProgram = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	glGetIntegerv(GL_SCISSOR_BOX, s.clipRect.data());
	checkOpenGLError("");

	s.depthTestEnabled = glIsEnabled(GL_DEPTH_TEST) != 0;
	s.polygonOffsetFillEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL) != 0;
	s.cullEnabled = glIsEnabled(GL_CULL_FACE) != 0;
	s.alphaToCoverage = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE) != 0;
	
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &s.polygonOffsetFactor);
	checkOpenGLError("");
	
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &s.polygonOffsetUnits);
	checkOpenGLError("");
	
	GLboolean bValue = 0;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &bValue);
	s.depthMask = bValue != 0;
	checkOpenGLError("");

	GLboolean cValue[4] = { };
	glGetBooleanv(GL_COLOR_WRITEMASK, cValue);
	s.colorMask = (cValue[0] * static_cast<size_t>(ColorMask::Red)) | (cValue[1] * static_cast<size_t>(ColorMask::Green)) |
		(cValue[2] * static_cast<size_t>(ColorMask::Blue)) | (cValue[3] * static_cast<size_t>(ColorMask::Alpha));
	checkOpenGLError("");

	s.clipEnabled = glIsEnabled(GL_SCISSOR_TEST) != 0;
	checkOpenGLError("");

	value = 0;
	glGetIntegerv(GL_CULL_FACE_MODE, &value);
	if (value == GL_FRONT)
		s.lastCull = CullState::Front;
	else if (value == GL_BACK)
		s.lastCull = CullState::Back;
	else
		ET_FAIL("Invalid cull state retreived");
	checkOpenGLError("");

	vec4i vp;
	glGetIntegerv(GL_VIEWPORT, vp.data());
	s.viewportSize = vec2i(vp.z, vp.w);
	s.viewportSizeFloat = vec2(static_cast<float>(s.viewportSize.x), static_cast<float>(s.viewportSize.y));
	s.mainViewportSize = s.viewportSize;
	s.mainViewportSizeFloat = s.viewportSizeFloat;
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_DEPTH_FUNC, &value);
	checkOpenGLError("");
	
	if (value == GL_NEVER)
		s.lastDepthFunc = DepthFunc::Never;
	if (value == GL_LESS)
		s.lastDepthFunc = DepthFunc::Less;
	else if (value == GL_LEQUAL)
		s.lastDepthFunc = DepthFunc::LessOrEqual;
	else if (value == GL_EQUAL)
		s.lastDepthFunc = DepthFunc::Equal;
	else if (value == GL_GEQUAL)
		s.lastDepthFunc = DepthFunc::GreaterOrEqual;
	else if (value == GL_GREATER)
		s.lastDepthFunc = DepthFunc::Greater;
	else if (value == GL_ALWAYS)
		s.lastDepthFunc = DepthFunc::Always;
	else
		ET_FAIL("Unknown GL_DEPTH_FUNC value");

	glGetFloatv(GL_COLOR_CLEAR_VALUE, s.clearColor.data());
	checkOpenGLError("");
	
	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &s.clearDepth);
	checkOpenGLError("");

	// TODO: get this from state, too lazy now.
	s.wireframe = false;
	
	s.blendEnabled = glIsEnabled(GL_BLEND) != 0;
	checkOpenGLError("");
	
	int blendSource = 0;
	glGetIntegerv(GL_BLEND_SRC_RGB, &blendSource);
	checkOpenGLError("");
	
	int blendDest = 0;
	glGetIntegerv(GL_BLEND_DST_RGB, &blendDest);
	checkOpenGLError("");
	
	s.lastColorBlend = blendValuesToBlendState(blendSource, blendDest);

	blendSource = 0;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSource);
	checkOpenGLError("");
	
	blendDest = 0;
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDest);
	checkOpenGLError("");
	
	s.lastAlphaBlend = blendValuesToBlendState(blendSource, blendDest);
#endif
	
	return s;
}
