/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/opengl/opengl.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/opengl/opengl_renderer.h>
#include <et/rendering/opengl/opengl_renderpass.h>
#include <et/rendering/opengl/opengl_texture.h>
#include <et/rendering/opengl/opengl_vertexbuffer.h>
#include <et/rendering/opengl/opengl_indexbuffer.h>
#include <et/rendering/opengl/opengl_program.h>
#include <et/rendering/opengl/opengl_pipelinestate.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/material.h>
#include <et/rendering/base/indexarray.h>
#include <et/rendering/base/vertexstorage.h>

#if (ET_PLATFORM_MAC)
#	include <OpenGL/OpenGL.h>
#	include <OpenGL/CGLTypes.h>
#endif

namespace et
{

class OpenGLRendererPrivate
{
public:
#if (ET_PLATFORM_MAC)
	CGLPixelFormatObj glPixelFormat = nullptr;
	CGLContextObj glContext = nullptr;
#elif (ET_PLATFORM_WIN)
	HDC dc = nullptr;
#endif
};

OpenGLRenderer::OpenGLRenderer(RenderContext* rc)
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(OpenGLRenderer);
}

OpenGLRenderer::~OpenGLRenderer()
{
	ET_PIMPL_FINALIZE(OpenGLRenderer)
}

void OpenGLRenderer::init(const RenderContextParameters& params)
{
#if (ET_PLATFORM_MAC)
	
	bool msaaEnabled = params.multisamplingQuality != MultisamplingQuality::None;
	CGLPixelFormatAttribute attribs[128] =
	{
		kCGLPFADoubleBuffer,
		kCGLPFAColorSize, CGLPixelFormatAttribute(24),
		kCGLPFAAlphaSize, CGLPixelFormatAttribute(8),
		kCGLPFADepthSize, CGLPixelFormatAttribute(32),
		kCGLPFABackingStore, CGLPixelFormatAttribute(1),
		kCGLPFAAccelerated,
		kCGLPFAOpenGLProfile, CGLPixelFormatAttribute(kCGLOGLPVersion_GL4_Core),
	};

	size_t msaaFirstEntry = 0;
	while (attribs[++msaaFirstEntry]);

	if (msaaEnabled)
	{
		attribs[msaaFirstEntry+0] = kCGLPFAMultisample;
		attribs[msaaFirstEntry+1] = kCGLPFASampleBuffers;
		attribs[msaaFirstEntry+2] = CGLPixelFormatAttribute(1);
		attribs[msaaFirstEntry+3] = kCGLPFASamples;
		attribs[msaaFirstEntry+4] = CGLPixelFormatAttribute(32);
	}

	GLint numPixelFormats = 0;
	auto err = CGLChoosePixelFormat(attribs, &_private->glPixelFormat, &numPixelFormats);
	ET_ASSERT(err == kCGLNoError);

	err = CGLCreateContext(_private->glPixelFormat, nullptr, &_private->glContext);
	ET_ASSERT(err == kCGLNoError);

	CGLSetCurrentContext(_private->glContext);
	GLint swap = static_cast<GLint>(params.swapInterval);
	CGLSetParameter(_private->glContext, kCGLCPSwapInterval, &swap);

	application().context().objects[3] = _private->glPixelFormat;
	application().context().objects[4] = _private->glContext;

#elif (ET_PLATFORM_WIN)

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };
	pfd.nVersion = 1;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;

	_private->dc = reinterpret_cast<HDC>(application().context().objects[1]);
	
	int pixelFormat = ChoosePixelFormat(_private->dc, &pfd);
	ET_ASSERT(pixelFormat != 0);

	SetPixelFormat(_private->dc, pixelFormat, &pfd);

	HGLRC tempContext = wglCreateContext(_private->dc);
	wglMakeCurrent(_private->dc, tempContext);

	auto local_wglCreateContextAttribsARB = (GLEEPFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	int contextAttribs[] = 
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, params.contextTargetVersion.x,
		WGL_CONTEXT_MINOR_VERSION_ARB, params.contextTargetVersion.y,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	HGLRC glRC = local_wglCreateContextAttribsARB(_private->dc, nullptr, contextAttribs);
	wglMakeCurrent(_private->dc, glRC);
	wglDeleteContext(tempContext);

	application().context().objects[2] = glRC;

#endif

	OpenGLCapabilities::instance().checkCaps();
}

void OpenGLRenderer::shutdown()
{
#if (ET_PLATFORM_MAC)
	CGLDestroyContext(_private->glContext);
	CGLDestroyPixelFormat(_private->glPixelFormat);
#endif
}

void OpenGLRenderer::begin()
{
#if (ET_PLATFORM_MAC)
	CGLSetCurrentContext(_private->glContext);
	CGLLockContext(_private->glContext);
#elif (ET_PLATFORM_WIN)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void OpenGLRenderer::present()
{
	checkOpenGLError("OpenGLRenderer::present()");

#if (ET_PLATFORM_MAC)
	ET_ASSERT(CGLGetCurrentContext() == _private->glContext);
	CGLFlushDrawable(_private->glContext);
	CGLUnlockContext(_private->glContext);
#elif (ET_PLATFORM_WIN)
	SwapBuffers(_private->dc);
#endif
}

RenderPass::Pointer OpenGLRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	OpenGLRenderPass::Pointer result = OpenGLRenderPass::Pointer::create(info);

	return result;
}

void OpenGLRenderer::submitRenderPass(RenderPass::Pointer in_pass)
{
	/*
	 * TODO : implement
	 *
	OpenGLRenderPass::Pointer pass = in_pass;

	GLbitfield clearMask = 0;
	const RenderPass::Target& rt = pass->info().target;

	if (rt.colorLoadOperation == FramebufferOperation::Clear)
	{
		clearMask |= GL_COLOR_BUFFER_BIT;
		const auto& clr = rt.clearColor;
		glClearColor(clr.x, clr.y, clr.z, clr.w);
	}

	if (rt.depthLoadOperation == FramebufferOperation::Clear)
	{
		clearMask |= GL_DEPTH_BUFFER_BIT;
		glClearDepth(rt.clearDepth);
	}

	if (rt.destination.valid())
	{
		rt.destination->bind();
		etViewport(0, 0, rt.destination->size().x, rt.destination->size().y);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		etViewport(0, 0, rc()->size().x, rc()->size().y);
	}

	if (clearMask != 0)
	{
		glClear(clearMask);
	}

	for (auto& batch_ptr : pass->mutableRenderBatches())
	{
		auto& batch = batch_ptr.reference();
		auto& mat = batch.material().reference();
		auto& prog = mat.program().reference();
		auto& vao = batch.vao().reference();
		auto& ib = vao.indexBuffer().reference();

		mat.enableSnapshotInRenderState(rc()->renderState(), batch.materialSnapshot());
		prog.setTransformMatrix(batch.transformation(), false);
		prog.setCameraProperties(pass->info().camera);
		prog.setDefaultLightPosition(pass->info().defaultLightPosition, false);
		
		vao.bind();
		drawIndexedPrimitive(ib.primitiveType(), ib.format(), batch.firstIndex(), batch.numIndexes());
	}
	*/
}

/*
 * Vertex buffers
 */
VertexBuffer::Pointer OpenGLRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return OpenGLVertexBuffer::Pointer::create(vs->declaration(), vs->data(), dt, name);
}

IndexBuffer::Pointer OpenGLRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return OpenGLIndexBuffer::Pointer::create(ia, dt, name);
}

/*
 * Textures
 */
Texture::Pointer OpenGLRenderer::createTexture(TextureDescription::Pointer desc)
{
    return OpenGLTexture::Pointer::create(desc);
}
    
/*
 * Programs
 */
Program::Pointer OpenGLRenderer::createProgram(const std::string& source, const std::string&,
	const StringList& defines, const std::string& baseFolder)
{
    std::string vertexSource = OpenGLProgram::commonHeader() + OpenGLProgram::vertexShaderHeader();
    std::string fragmentSource = OpenGLProgram::commonHeader() + OpenGLProgram::fragmentShaderHeader();
    
    for (const std::string& define : defines)
    {
        vertexSource += "\n" + define + "\n";
        fragmentSource += "\n" + define + "\n";
    }
    
    vertexSource += source;
    fragmentSource += source;
    
    parseIncludes(vertexSource, baseFolder);
    parseIncludes(fragmentSource, baseFolder);
    
    OpenGLProgram::Pointer program = OpenGLProgram::Pointer::create();
    program->build(vertexSource, fragmentSource);
    return program;
}

/*
 * Pipeline state
 */
PipelineState::Pointer OpenGLRenderer::createPipelineState(RenderPass::Pointer, Material::Pointer, VertexStream::Pointer)
{
	return OpenGLPipelineState::Pointer::create();
}

}


/*
extern const std::string fullscreen_vertex_shader; 
extern const std::string fullscreen_scaled_vertex_shader;
extern const std::string scaled_copy_vertex_shader;
extern const std::string scaled_rotated_copy_vertex_shader;
extern const std::string copy_fragment_shader;
extern const std::string depth_fragment_shader;

Renderer::Renderer(RenderContext* rc) :
	_rc(rc)
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

	_fullscreenQuadVao = rc->renderer()->createVertexArrayObject("__et__internal__fullscreen_vao__",
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
}

void Renderer::fullscreenPass()
{
	_rc->renderState()->bindVertexArrayObject(_fullscreenQuadVao);
	drawAllElements(_fullscreenQuadVao->indexBuffer());
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture, const vec4& tint)
{
	auto prog = _fullscreenProgram[static_cast<int32_t>(texture->target())];

	_rc->renderState()->bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState()->bindProgram(prog);
	prog->setUniform("color_texture_size", texture->sizeFloat());
	prog->setUniform("tint", tint);
	fullscreenPass();
}

void Renderer::renderFullscreenDepthTexture(const Texture::Pointer& texture, float factor)
{
	_rc->renderState()->bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState()->bindProgram(_fullscreenDepthProgram);
	_fullscreenDepthProgram->setUniform(_fullScreenDepthProgram_FactorUniform, factor);
	fullscreenPass();
}

void Renderer::renderFullscreenTexture(const Texture::Pointer& texture, const vec2& scale, const vec4& tint)
{
	_rc->renderState()->bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState()->bindProgram(_fullscreenScaledProgram);
	_scaledProgram->setUniform(_fullScreenScaledProgram_PSUniform, scale);
	_scaledProgram->setUniform(_fullScreenScaledProgram_TintUniform, tint);
	fullscreenPass();
}

void Renderer::renderTexture(const Texture::Pointer& texture, const vec2& position, const vec2& size, const vec4& tint)
{
	_rc->renderState()->bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState()->bindProgram(_scaledProgram);
	_scaledProgram->setUniform(_scaledProgram_PSUniform, vec4(position, size));
	_scaledProgram->setUniform(_scaledProgram_TintUniform, tint);
	fullscreenPass();
}

void Renderer::renderTextureRotated(const Texture::Pointer& texture, float angle, const vec2& position,
	const vec2& size, const vec4& tint)
{
	_rc->renderState()->bindTexture(_defaultTextureBindingUnit, texture);
	_rc->renderState()->bindProgram(_scaledRotatedProgram);
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_PSUniform, vec4(position, size));
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_TintUniform, tint);
	_scaledRotatedProgram->setUniform(_scaledRotatedProgram_AngleUniform, angle);
	fullscreenPass();
}

vec2 Renderer::currentViewportCoordinatesToScene(const vec2i& coord)
{
	auto vpSize = vector2ToFloat(_rc->renderState()->viewportSize());
	return vec2(2.0f * static_cast<float>(coord.x) / vpSize.x - 1.0f,
		1.0f - 2.0f * static_cast<float>(coord.y) / vpSize.y );
}

vec2 Renderer::currentViewportSizeToScene(const vec2i& size)
{
	auto vpSize = vector2ToFloat(_rc->renderState()->viewportSize());
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
	RenderState::Pointer rs = _rc->renderState();
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
 
*/

/*
 * Default shaders
 */

/*
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
*/
