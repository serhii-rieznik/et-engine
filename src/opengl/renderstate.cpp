/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/vertexbuffer/vertexdeclaration.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/renderstate.h>

using namespace et;

RenderState::Descriptor::Descriptor()
{
	cache.drawBuffers.fill(0);
	cache.enabledVertexAttributes.fill(0);
}

PreservedRenderStateScope::PreservedRenderStateScope(RenderContext* rc, bool shouldApplyBefore) :
	_rc(rc), _desc(RenderState::currentState())
{
	if (shouldApplyBefore)
		_rc->renderState().applyState(_desc);
}

PreservedRenderStateScope::~PreservedRenderStateScope()
{
	_rc->renderState().applyState(_desc);
}

void RenderState::setRenderContext(RenderContext* rc)
{
	_rc = rc;
	_desc = RenderState::currentState();
	auto currentUnit = _desc.cache.boundTextures[TextureTarget::Texture_2D][_desc.cache.activeTextureUnit];
	bindTexture(_desc.cache.activeTextureUnit, currentUnit, TextureTarget::Texture_2D);
}

void RenderState::setMainViewportSize(const vec2i& sz, bool force)
{
	if (!force && (sz.x == _mainViewport.width) && (sz.y == _mainViewport.height)) return;

	_mainViewport.setSize(sz);
	_defaultFramebuffer->forceSize(sz);

	bool shouldSetViewport = (_desc.cache.boundFramebuffer == 0) ||
		(_defaultFramebuffer.valid() && (_desc.cache.boundFramebuffer == _defaultFramebuffer->apiHandle()));
	
	if (shouldSetViewport)
	{
		etViewport(0, 0, _mainViewport.width, _mainViewport.height);
	}
}

void RenderState::setViewportSize(const vec2i& sz, bool force)
{
	if (!force && (sz.x == _desc.cache.viewport.width) && (sz.y == _desc.cache.viewport.height)) return;

	_desc.cache.viewport.setSize(sz);
	etViewport(0, 0, _desc.cache.viewport.width, _desc.cache.viewport.height);
}

void RenderState::setActiveTextureUnit(uint32_t unit, bool force)
{
	if ((unit != _desc.cache.activeTextureUnit) || force)
	{
		_desc.cache.activeTextureUnit = unit;
		
		glActiveTexture(GL_TEXTURE0 + _desc.cache.activeTextureUnit);
		checkOpenGLError("glActiveTexture(GL_TEXTURE0 + %u)", _desc.cache.activeTextureUnit);
	}
}

void RenderState::bindTexture(uint32_t unit, uint32_t texture, TextureTarget target, bool force)
{
	setActiveTextureUnit(unit, force);
	
	if (force || (_desc.cache.boundTextures[target][unit] != texture))
	{
		_desc.cache.boundTextures[target][unit] = texture;
		etBindTexture(textureTargetValue(target), texture);
	}
}

void RenderState::bindProgram(uint32_t program, bool force)
{
	if (force || (program != _desc.cache.boundProgram))
	{ 
		_desc.cache.boundProgram = program;
		etUseProgram(program);
	}
}

void RenderState::bindProgram(const Program::Pointer& prog, bool force)
{
	ET_ASSERT(prog.valid());
	bindProgram(static_cast<uint32_t>(prog->apiHandle()), force);
}

void RenderState::bindBuffer(uint32_t target, uint32_t buffer, bool force)
{ 
	if ((target == GL_ARRAY_BUFFER) && (force || (_desc.cache.boundArrayBuffer != buffer)))
	{
		_desc.cache.boundArrayBuffer = buffer;
		etBindBuffer(target, buffer);
	}
	else if ((target == GL_ELEMENT_ARRAY_BUFFER) && (force || (_desc.cache.boundElementArrayBuffer != buffer)))
	{ 
		_desc.cache.boundElementArrayBuffer = buffer;
		etBindBuffer(target, buffer);
	}
	else if ((target != GL_ARRAY_BUFFER) && (target != GL_ELEMENT_ARRAY_BUFFER))
	{
		log::warning("Trying to bind buffer %u to unknown target %u", buffer, target);
	}
}

void RenderState::setVertexAttributes(const VertexDeclaration& decl, bool force)
{
	for (uint32_t i = 0; i < VertexAttributeUsage_max; ++i)
		setVertexAttribEnabled(i, decl.has(static_cast<VertexAttributeUsage>(i)), force);
	
	setVertexAttributesBaseIndex(decl, 0);
}

void RenderState::setVertexAttributesBaseIndex(const VertexDeclaration& decl, uint32_t index, bool force)
{
	for (size_t i = 0; i < decl.numElements(); ++i)
	{
		const VertexElement& e = decl.element(i);
		uint32_t dataOffset = index * (decl.interleaved() ? decl.dataSize() : dataTypeSize(e.type()));
		setVertexAttribPointer(e, dataOffset, force);
	}
}

void RenderState::bindBuffer(const VertexBuffer::Pointer& buffer, bool force)
{
	if (buffer.valid())
	{
		bindBuffer(GL_ARRAY_BUFFER, static_cast<uint32_t>(buffer->apiHandle()), force);
		setVertexAttributes(buffer->declaration(), force);
	}
	else
	{
		bindBuffer(GL_ARRAY_BUFFER, 0, force);
	}
}

void RenderState::bindBuffer(const IndexBuffer::Pointer& buf, bool force)
{
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.valid() ? static_cast<uint32_t>(buf->apiHandle()) : 0, force);
}

void RenderState::bindBuffers(const VertexBuffer::Pointer& vb, const IndexBuffer::Pointer& ib, bool force)
{
	bindBuffer(vb, force);
	bindBuffer(ib, force);
}

void RenderState::bindVertexArrayObject(uint32_t buffer, bool force)
{
	ET_ASSERT(OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects));
	
	if (force || (_desc.cache.boundVertexArrayObject != buffer))
	{
		_desc.cache.boundVertexArrayObject = buffer;
		etbindVertexArrayObject(buffer);
	}
}

void RenderState::bindVertexArrayObject(const VertexArrayObject::Pointer& vao, bool force)
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		bindVertexArrayObject(vao.valid() ? static_cast<uint32_t>(vao->apiHandle()) : 0, force);
	}
	else
	{
		if (vao.valid())
			bindBuffers(vao->vertexBuffer(), vao->indexBuffer(), force);
		else
			bindBuffers(VertexBuffer::Pointer(), IndexBuffer::Pointer(), force);
	}
}

void RenderState::resetBufferBindings()
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		bindVertexArrayObject(0);
	
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	bindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderState::bindTexture(uint32_t unit, const Texture::Pointer& texture, bool force)
{
	if (texture.valid())
		bindTexture(unit, static_cast<uint32_t>(texture->apiHandle()), texture->target(), force);
	else
		bindTexture(unit, 0, TextureTarget::Texture_2D, force);
}

void RenderState::bindFramebuffer(uint32_t framebuffer, bool force)
{
	bindFramebuffer(framebuffer, GL_FRAMEBUFFER, force);
}

void RenderState::bindFramebuffer(uint32_t framebuffer, uint32_t target, bool force)
{
	if (force || (_desc.cache.boundFramebuffer != framebuffer) ||
		(_desc.cache.boundDrawFramebuffer != framebuffer) ||
		(_desc.cache.boundReadFramebuffer != framebuffer))
	{
		_desc.cache.boundDrawFramebuffer = framebuffer;
		_desc.cache.boundReadFramebuffer = framebuffer;
		_desc.cache.boundFramebuffer = framebuffer;
		etBindFramebuffer(target, framebuffer);
	}
}

void RenderState::bindReadFramebuffer(uint32_t framebuffer, bool force)
{
	bool alreadyBound =
		(_desc.cache.boundReadFramebuffer == framebuffer) ||
		(_desc.cache.boundFramebuffer == framebuffer);
	
	if (force || !alreadyBound)
	{
		_desc.cache.boundReadFramebuffer = framebuffer;
		etBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	}
}

void RenderState::bindDrawFramebuffer(uint32_t framebuffer, bool force)
{
	bool alreadyBound =
		(_desc.cache.boundDrawFramebuffer == framebuffer) ||
		(_desc.cache.boundFramebuffer == framebuffer);
	
	if (force || !alreadyBound)
	{
		_desc.cache.boundDrawFramebuffer = framebuffer;
		etBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	}
}

void RenderState::bindFramebuffer(const Framebuffer::Pointer& fbo, bool force)
{
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
		setViewportSize(_mainViewport.size(), force);
	}
}

void RenderState::bindRenderbuffer(uint32_t renderbuffer, bool force)
{
	if (force || (_desc.cache.boundRenderbuffer != renderbuffer))
	{
		_desc.cache.boundRenderbuffer = renderbuffer;
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		checkOpenGLError("glBindRenderbuffer");
	}
}

void RenderState::setDefaultFramebuffer(const Framebuffer::Pointer& framebuffer)
{
	_defaultFramebuffer = framebuffer;

	if (_defaultFramebuffer.valid())
		setMainViewportSize(_defaultFramebuffer->size());
}

void RenderState::bindDefaultFramebuffer(bool force)
{
	bindFramebuffer(_defaultFramebuffer, force);
}

void RenderState::setDrawBuffersCount(uint32_t count)
{
	glDrawBuffers(count, drawBufferTargets());
	checkOpenGLError("glDrawBuffers(%d, ...)", count);
}

void RenderState::setDepthWriteEnabled(bool enable, bool force)
{
	if (force || (_desc.depth.depthWriteEnabled != enable))
	{
		_desc.depth.depthWriteEnabled = enable;
		glDepthMask(enable);
	}
}

void RenderState::setDepthTestEnabled(bool enable, bool force)
{
	if (force || (enable != _desc.depth.depthTestEnabled))
	{
		_desc.depth.depthTestEnabled = enable;
		(enable ? glEnable : glDisable)(GL_DEPTH_TEST);
	}
}

void RenderState::setDepthFunc(CompareFunction func, bool force)
{
	if (force || (func != _desc.depth.compareFunction))
	{
		_desc.depth.compareFunction = func;
		glDepthFunc(compareFunctionValue(func));
	}
}

void RenderState::setBlendState(const BlendState& bs, bool force)
{
	if ((_desc.blend.blendEnabled != bs.blendEnabled) || force)
	{
		(bs.blendEnabled ? glEnable : glDisable)(GL_BLEND);
		_desc.blend.blendEnabled = bs.blendEnabled;
	}
	
	if ((bs.color != _desc.blend.color) || (bs.alpha != _desc.blend.alpha) || force)
	{
		glBlendFuncSeparate(blendFunctionValue(bs.color.source), blendFunctionValue(bs.color.dest),
			blendFunctionValue(bs.alpha.source), blendFunctionValue(bs.alpha.dest));
		_desc.blend.color = bs.color;
		_desc.blend.alpha = bs.alpha;
	}
	
	if ((bs.colorOperation != _desc.blend.colorOperation) || (bs.alphaOperation != _desc.blend.alphaOperation) || force)
	{
		glBlendEquationSeparate(blendOperationValue(bs.colorOperation), blendOperationValue(bs.alphaOperation));
		_desc.blend.colorOperation = bs.colorOperation;
		_desc.blend.alphaOperation = bs.alphaOperation;
	}
}

void RenderState::setDepthState(const DepthState& state, bool force)
{
	setDepthWriteEnabled(state.depthWriteEnabled);
	setDepthTestEnabled(state.depthTestEnabled);
	setDepthClearValue(state.clearDepth);
	setDepthFunc(state.compareFunction);
}

void RenderState::setBlendConfiguration(et::BlendConfiguration blend, bool force)
{
	ET_ASSERT(blend < BlendConfiguration::max);
	setBlendState(blendConfigurationToBlendState(blend), force);
}

void RenderState::vertexArrayDeleted(uint32_t buffer)
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		ET_ASSERT(_desc.cache.boundVertexArrayObject == buffer);
		(void)buffer;
		
		bindBuffer(GL_ARRAY_BUFFER, 0, true);
		bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0, true);
		bindVertexArrayObject(0);
	}
}

void RenderState::vertexBufferDeleted(uint32_t buffer)
{
	if (_desc.cache.boundArrayBuffer == buffer)
		bindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderState::indexBufferDeleted(uint32_t buffer)
{
	if (_desc.cache.boundElementArrayBuffer == buffer)
		bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RenderState::programDeleted(uint32_t program)
{
	if (_desc.cache.boundProgram == program)
		bindProgram(0, true);
}

void RenderState::textureDeleted(uint32_t texture)
{
	for (auto& target : _desc.cache.boundTextures)
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
	if (_defaultFramebuffer.valid() && (_defaultFramebuffer->apiHandle() == buffer))
		_defaultFramebuffer = Framebuffer::Pointer();
	
	if (_desc.cache.boundFramebuffer == buffer)
		bindDefaultFramebuffer();
}

void RenderState::setVertexAttribEnabled(uint32_t attrib, bool enabled, bool force)
{
	bool wasEnabled = _desc.cache.enabledVertexAttributes[attrib] > 0;

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
	
	_desc.cache.enabledVertexAttributes[attrib] = enabled;
}

void RenderState::setVertexAttribPointer(const VertexElement& e, uint32_t baseIndex, bool force)
{
	(void)force;
	
	if (e.dataFormat() == DataFormat::Int)
	{
		glVertexAttribIPointer(GLuint(e.usage()), static_cast<GLint>(e.components()), dataFormatValue(e.dataFormat()),
			e.stride(), reinterpret_cast<GLvoid*>(uintptr_t(e.offset() + baseIndex)));
	}
	else if (e.dataFormat() == DataFormat::Float)
	{
		glVertexAttribPointer(GLuint(e.usage()), static_cast<GLint>(e.components()), dataFormatValue(e.dataFormat()),
			false, e.stride(), reinterpret_cast<GLvoid*>(uintptr_t(e.offset() + baseIndex)));
	}
	else
	{
		ET_FAIL("Unhandled vertex attribute data type.");
	}

	checkOpenGLError("glVertexAttribPointer");
}

void RenderState::setCulling(et::CullMode cull, bool force)
{
	ET_ASSERT(static_cast<uint32_t>(cull) < CullMode_max);
	
	static const GLenum cullStates[CullMode_max] =
		{ GL_NONE, GL_BACK, GL_FRONT };
	
	if ((_desc.rasterizer.cullMode != cull) || force)
	{
		if (cull == CullMode::Disabled)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(cullStates[static_cast<uint32_t>(cull)]);
		}
		_desc.rasterizer.cullMode = cull;
		checkOpenGLError("RenderState::setCulling");
	}
}

void RenderState::setDepthBias(bool enabled, float bias, float slope, bool force)
{
	if ((_desc.rasterizer.depthBiasEnabled != enabled) || force)
	{
		_desc.rasterizer.depthBiasEnabled = enabled;
		(enabled ? glEnable : glDisable)(GL_POLYGON_OFFSET_FILL);
	}
	
	if ((_desc.rasterizer.depthBias != bias) || (_desc.rasterizer.depthSlopeScale != slope) || force)
	{
		_desc.rasterizer.depthBias = bias;
		_desc.rasterizer.depthSlopeScale = slope;
		glPolygonOffset(slope, bias);
	}
}

void RenderState::setFillMode(FillMode mode, bool force)
{
#if (ET_OPENGLES)
	log::error("Invalid call in OpenGL ES");
#else
	if ((_desc.rasterizer.fillMode != mode) || force)
	{
		_desc.rasterizer.fillMode = mode;
		glPolygonMode(GL_FRONT_AND_BACK, wireframeRendering() ? GL_LINE : GL_FILL);
	}
#endif
}

void RenderState::setClearColor(const vec4& color, bool force)
{
	if (force || ((_desc.rasterizer.clearColor - color).dotSelf() > std::numeric_limits<float>::epsilon()))
	{
		_desc.rasterizer.clearColor = color;
		glClearColor(color.x, color.y, color.z, color.w);
		checkOpenGLError("RenderState::glClearColor");
	}
}

void RenderState::setColorMask(uint32_t mask, bool force)
{
	if (force || (_desc.rasterizer.colorMask != mask))
	{
		_desc.rasterizer.colorMask = mask;
		GLboolean redEnabled = (mask & static_cast<uint32_t>(ColorMask::Red)) != 0;
		GLboolean greenEnabled = (mask & static_cast<uint32_t>(ColorMask::Green)) != 0;
		GLboolean blueEnabled = (mask & static_cast<uint32_t>(ColorMask::Blue)) != 0;
		GLboolean alphaEnabled = (mask & static_cast<uint32_t>(ColorMask::Alpha)) != 0;
		glColorMask(redEnabled, greenEnabled, blueEnabled, alphaEnabled);
		checkOpenGLError("RenderState::setColorMask");
	}
}

void RenderState::setDepthClearValue(float depth, bool force)
{
	if (force || (_desc.depth.clearDepth != depth))
	{
		_desc.depth.clearDepth = depth;
		glClearDepth(depth);
		checkOpenGLError("RenderState::setClearDepth");
	}
}

void RenderState::setScissor(bool enable, const recti& clip, bool force)
{
	if (force || (enable != _desc.rasterizer.scissorEnabled))
	{
		_desc.rasterizer.scissorEnabled = enable;
		(enable ? glEnable : glDisable)(GL_SCISSOR_TEST);
		checkOpenGLError("RenderState::setClip");
	}

	if (force || (clip != _desc.rasterizer.scissorRectangle))
	{
		_desc.rasterizer.scissorRectangle = clip;
		glScissor(clip.left, clip.top, etMax(0, clip.width), etMax(0, clip.height));
		checkOpenGLError("RenderState::setClip - glScissor");
	}
}

void RenderState::setSampleAlphaToCoverage(bool enable, bool force)
{
	if (force || (_desc.blend.alphaToCoverageEnabled != enable))
	{
		_desc.blend.alphaToCoverageEnabled = enable;
		(enable ? glEnable : glDisable)(GL_SAMPLE_ALPHA_TO_COVERAGE);
		checkOpenGLError("setSampleAlphaToCoverage(%s)", enable ? "true" : "false");
	}
}

void RenderState::applyState(const RenderState::Descriptor& s)
{
	setBlendState(s.blend, true);
	setDepthState(s.depth, true);
	
	setClearColor(s.rasterizer.clearColor, true);
	setColorMask(s.rasterizer.colorMask, true);
	setDepthBias(s.rasterizer.depthBiasEnabled, s.rasterizer.depthBias, s.rasterizer.depthSlopeScale, true);
	setFillMode(s.rasterizer.fillMode, true);
	setCulling(s.rasterizer.cullMode, true);
	setViewportSize(s.cache.viewport.size(), true);
	
	bindFramebuffer(s.cache.boundFramebuffer, true);
	bindProgram(s.cache.boundProgram, true);
	bindVertexArrayObject(s.cache.boundVertexArrayObject, true);
	bindBuffer(GL_ELEMENT_ARRAY_BUFFER, s.cache.boundElementArrayBuffer, true);
	bindBuffer(GL_ARRAY_BUFFER, s.cache.boundArrayBuffer, true);
	
	for (auto& target : s.cache.boundTextures)
	{
		for (auto& unit : target.second)
			bindTexture(unit.first, unit.second, target.first, true);
	}
	
	for (uint32_t i = 0, e = static_cast<uint32_t>(VertexAttributeUsage::max); i < e; ++i)
		setVertexAttribEnabled(i, (s.cache.enabledVertexAttributes[i] != 0), true);
	
	setActiveTextureUnit(s.cache.activeTextureUnit, true);
}

BlendState RenderState::currentBlendState()
{
	BlendState blend;
	
	blend.alphaToCoverageEnabled = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE) != 0;
	blend.blendEnabled = glIsEnabled(GL_BLEND) != 0;
	
	int blendSource = 0;
	glGetIntegerv(GL_BLEND_SRC_RGB, &blendSource);
	blend.color.source = valueToBlendFunction(blendSource);
	
	int blendDest = 0;
	glGetIntegerv(GL_BLEND_DST_RGB, &blendDest);
	blend.color.dest = valueToBlendFunction(blendDest);
	
	blendSource = 0;
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSource);
	blend.alpha.source = valueToBlendFunction(blendSource);
	
	blendDest = 0;
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDest);
	blend.alpha.dest = valueToBlendFunction(blendDest);
	
	int value = 0;
	glGetIntegerv(GL_BLEND_EQUATION_RGB, &value);
	blend.colorOperation = valueToBlendOperation(value);
	
	value = 0;
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &value);
	blend.alphaOperation = valueToBlendOperation(value);
	
	return blend;
}

DepthState RenderState::currentDepthState()
{
	DepthState depth;

	GLboolean bValue = 0;
	int value = 0;
	
	depth.depthTestEnabled = glIsEnabled(GL_DEPTH_TEST) != 0;
	
	glGetBooleanv(GL_DEPTH_WRITEMASK, &bValue);
	depth.depthWriteEnabled = (bValue != 0);
	glGetIntegerv(GL_DEPTH_FUNC, &value);
	depth.compareFunction = valueToCompareFunction(value);

	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &depth.clearDepth);
	
	return depth;
}

RasterizerState RenderState::currentRasterizedState()
{
	RasterizerState rasterizer;
	
	glGetIntegerv(GL_SCISSOR_BOX, rasterizer.scissorRectangle.data());
	checkOpenGLError("glGetIntegerv(GL_SCISSOR_BOX, ...)");
	
	rasterizer.depthBiasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL) != 0;
	
	if (glIsEnabled(GL_CULL_FACE))
	{
		int cullFaceMode = 0;
		glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);
		switch (cullFaceMode)
		{
			case GL_FRONT:
				rasterizer.cullMode = CullMode::Front;
				break;
			case GL_BACK:
				rasterizer.cullMode = CullMode::Back;
				break;
			default:
				log::error("Invalid or unsupported cull mode: %x", cullFaceMode);
				rasterizer.cullMode = CullMode::Disabled;
		}
	}
	else
	{
		rasterizer.cullMode = CullMode::Disabled;
	}
	
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &rasterizer.depthSlopeScale);
	checkOpenGLError("glGetFloatv(GL_POLYGON_OFFSET_FACTOR, ...)");
	
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &rasterizer.depthBias);
	checkOpenGLError("glGetFloatv(GL_POLYGON_OFFSET_UNITS, ...)");
	
	GLboolean cValue[4] = { };
	glGetBooleanv(GL_COLOR_WRITEMASK, cValue);
	rasterizer.colorMask =
		(cValue[0] * static_cast<uint32_t>(ColorMask::Red)) |
		(cValue[1] * static_cast<uint32_t>(ColorMask::Green)) |
		(cValue[2] * static_cast<uint32_t>(ColorMask::Blue)) |
		(cValue[3] * static_cast<uint32_t>(ColorMask::Alpha));
	checkOpenGLError("glGetBooleanv(GL_COLOR_WRITEMASK, ...)");
	
	rasterizer.scissorEnabled = glIsEnabled(GL_SCISSOR_TEST) != 0;
	checkOpenGLError("glIsEnabled(GL_SCISSOR_TEST)");
	
	glGetFloatv(GL_COLOR_CLEAR_VALUE, rasterizer.clearColor.data());
	checkOpenGLError("");
	
	int value = 0;
	glGetIntegerv(GL_POLYGON_MODE, &value);
	switch (value)
	{
	case GL_FILL:
		rasterizer.fillMode = FillMode::Solid;
		break;
	case GL_LINE:
		rasterizer.fillMode = FillMode::Wireframe;
		break;
	default:
		log::error("Invalid or unsupported fill mode: %x", value);
		rasterizer.fillMode = FillMode::Solid;
	}
	
	return rasterizer;
}

RenderStateCache RenderState::currentCacheValues()
{
	RenderStateCache cache;
	checkOpenGLError("currentState()");
	
	int value = 0;
	
	int maxDrawBuffers = 0;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	checkOpenGLError("");
	
	for (int i = 0; i < std::min(maxDrawBuffers, int(MaxRenderTargets)); ++i)
	{
		glGetIntegerv(GL_DRAW_BUFFER0 + i, &value);
		cache.drawBuffers[i] = value;
	}
	
	value = 0;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &value);
	cache.activeTextureUnit = static_cast<uint32_t>(value - GL_TEXTURE0);
	checkOpenGLError("");
	
	int maxTextureUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
	
	for (int i = 0; i < maxTextureUnits; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		
		value = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
		cache.boundTextures[TextureTarget::Texture_2D][i] = value;
		
		value = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
		cache.boundTextures[TextureTarget::Texture_Cube][i] = value;
	}
	
	glActiveTexture(GL_TEXTURE0 + cache.activeTextureUnit);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &value);
	cache.boundArrayBuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &value);
	cache.boundElementArrayBuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);
	cache.boundFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);
	cache.boundReadFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);
	cache.boundDrawFramebuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &value);
	cache.boundRenderbuffer = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	value = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &value);
	cache.boundVertexArrayObject = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	bool shouldLoadVertexAttribs = (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x) ||
	(cache.boundVertexArrayObject != 0);
	
	for (uint32_t i = 0, e = static_cast<uint32_t>(VertexAttributeUsage::max); shouldLoadVertexAttribs && (i < e); ++i)
	{
		int enabled = 0;
		glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
		cache.enabledVertexAttributes[i] = (enabled > 0);
		checkOpenGLError("glGetVertexAttribiv");
	}
	
	value = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &value);
	cache.boundProgram = static_cast<uint32_t>(value);
	checkOpenGLError("");
	
	glGetIntegerv(GL_VIEWPORT, cache.viewport.data());
	checkOpenGLError("glGetIntegerv(GL_VIEWPORT, ...)");
	
	return cache;
}

RenderState::Descriptor RenderState::currentState()
{
	RenderState::Descriptor s;
	s.blend = currentBlendState();
	s.depth = currentDepthState();
	s.rasterizer = currentRasterizedState();
	s.cache = currentCacheValues();
	return s;
}
