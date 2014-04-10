/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/gui/guirenderer.h>
#include <et/opengl/openglcaps.h>

using namespace et;
using namespace et::gui;

const size_t BlockSize = 2048;

extern std::string gui_default_vertex_src;
extern std::string gui_default_frag_src;
extern std::string gui_savefillrate_vertex_src;
extern std::string gui_savefillrate_frag_src;

GuiRenderer::GuiRenderer(RenderContext* rc, bool saveFillRate) :
	_rc(rc), _customAlpha(1.0f), _saveFillRate(saveFillRate)
{
	pushClipRect(recti(vec2i(0), rc->sizei()));

	if (_saveFillRate)
	{
		_guiProgram = rc->programFactory().genProgram("shader-gui", gui_savefillrate_vertex_src, std::string(),
			gui_savefillrate_frag_src);
	}
	else 
	{
		_guiProgram = rc->programFactory().genProgram("shader-gui", gui_default_vertex_src, std::string(),
			gui_default_frag_src, StringList());
	}
	
	_defaultTexture = rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, BinaryDataStorage(4, 0), "gui-default-texture");
	_defaultTexture->setFiltration(rc, TextureFiltration_Nearest, TextureFiltration_Nearest);

	_guiCustomOffsetUniform = _guiProgram->getUniformLocation("vCustomOffset");
	_guiCustomAlphaUniform = _guiProgram->getUniformLocation("customAlpha");
	_guiProgram->setUniform("layer0_texture", 0);
	
	if (!_saveFillRate)
		_guiProgram->setUniform("layer1_texture", 1);

	setProjectionMatrices(rc->size());
}

void GuiRenderer::resetClipRect()
{
	while (_clip.size() > 1)
		_clip.pop();
}

void GuiRenderer::pushClipRect(const recti& value)
{
	_clip.push(value);
}

void GuiRenderer::popClipRect()
{
	ET_ASSERT(_clip.size() > 1);
	_clip.pop();
}

void GuiRenderer::setProjectionMatrices(const vec2& contextSize)
{
	std::stack<recti> tempClipStack;
	
	while (_clip.size() > 1)
	{
		tempClipStack.push(_clip.top());
		_clip.pop();
	}
	
	_clip.pop();
	_clip.push(recti(vec2i(0), vec2i(static_cast<int>(contextSize.x), static_cast<int>(contextSize.y))));
	
	while (tempClipStack.size())
	{
		_clip.push(tempClipStack.top());
		tempClipStack.pop();
	}
	
	_defaultTransform = identityMatrix;
	_defaultTransform[0][0] =  2.0f / contextSize.x;
	_defaultTransform[1][1] = -2.0f / contextSize.y;
	_defaultTransform[3][0] = -1.0f;
	_defaultTransform[3][1] = 1.0f;
	_defaultTransform[3][3] = 1.0f;

	_guiCamera.perspectiveProjection(DEG_30, contextSize.aspect(), 0.1f, 10.0f);
	_guiCamera.lookAt(vec3(0.0f, 0.0f, std::cos(DEG_15) / std::sin(DEG_15)), vec3(0.0f), unitY);
}

void GuiRenderer::alloc(size_t count)
{
	if (_renderingElement.invalid()) return;

	size_t currentOffset = _renderingElement->_vertexList.lastElementIndex();
	size_t currentSize = _renderingElement->_vertexList.size();

	if (currentOffset + count >= currentSize)
	{
		size_t newSize = currentSize + BlockSize * (1 + count / BlockSize);
		_renderingElement->_vertexList.resize(newSize);
		_renderingElement->_indexArray->resize(newSize);
		_renderingElement->_indexArray->linearize(newSize);
	}
}

GuiVertexPointer GuiRenderer::allocateVertices(size_t count, const Texture& inTexture,
	ElementRepresentation cls, RenderLayer layer)
{
	if (!_renderingElement.valid()) return 0;
	
	Texture texture = inTexture.invalid() ? _defaultTexture : inTexture;

	bool shouldAdd = _renderingElement->_chunks.empty() || (_saveFillRate && (layer == RenderLayer_Layer1));
	
	if (_saveFillRate)
		layer = RenderLayer_Layer0;
	
	_renderingElement->_changed = true;
	size_t i0 = _renderingElement->_vertexList.lastElementIndex();

	if (_renderingElement->_chunks.size())
	{
		RenderChunk& lastChunk = _renderingElement->_chunks.back();
		
		bool sameConfiguration = (lastChunk.representation == cls) &&
			(lastChunk.layers[layer] == texture) && (lastChunk.clip == _clip.top());
		
		if (sameConfiguration)
			lastChunk.count += count;
		
		shouldAdd = !sameConfiguration;
	}

	if (shouldAdd)
	{
		_lastTextures[layer] = texture;
		_renderingElement->_chunks.push_back(RenderChunk(i0, count, 
			_lastTextures[RenderLayer_Layer0], _lastTextures[RenderLayer_Layer1], _clip.top(), cls));
	}
	
	alloc(count);
	
	_renderingElement->_vertexList.applyOffset(count);

	ET_ASSERT(i0 < _renderingElement->_vertexList.size());
	ET_ASSERT(i0 * _renderingElement->_vertexList.typeSize() < _renderingElement->_vertexList.dataSize());

	return _renderingElement->_vertexList.element_ptr(i0);
}

size_t GuiRenderer::addVertices(const GuiVertexList& vertices, const Texture& texture,
	ElementRepresentation cls, RenderLayer layer)
{
	size_t current = 0;
	size_t count = vertices.lastElementIndex();
	
	if (_renderingElement.valid() && (count > 0))
	{
		current = _renderingElement->_vertexList.lastElementIndex();
		GuiVertex* v0 = allocateVertices(count, texture, cls, layer);
		etCopyMemory(v0, vertices.data(), count * vertices.typeSize());
	}

	return current;
}

void GuiRenderer::setRendernigElement(const RenderingElement::Pointer& r)
{
	_renderingElement = r;
	_lastTextures[RenderLayer_Layer0] = _defaultTexture;
	_lastTextures[RenderLayer_Layer1] = _defaultTexture;
}

void GuiRenderer::beginRender(RenderContext* rc)
{
	rc->renderer()->clear(false, true);

	_depthTestEnabled = rc->renderState().depthTestEnabled();
	_blendEnabled = rc->renderState().blendEnabled();
	_blendState = rc->renderState().blendState();
	_depthMask = rc->renderState().depthMask();
	
	rc->renderState().setBlend(true, BlendState_Default);
	rc->renderState().bindProgram(_guiProgram);
}

void GuiRenderer::endRender(RenderContext* rc)
{
	rc->renderState().setDepthTest(_depthTestEnabled);
	rc->renderState().setBlend(_blendEnabled, static_cast<BlendState>(_blendState));
	rc->renderState().setDepthMask(_depthMask);
	rc->renderState().setClip(false, recti());
}

void GuiRenderer::render(RenderContext* rc)
{
	if (!_renderingElement.valid()) return;

	RenderState& rs = rc->renderState();
	Renderer* renderer = rc->renderer();

	_guiProgram->setUniform(_guiCustomOffsetUniform, GL_FLOAT_VEC2, _customOffset);
	_guiProgram->setUniform(_guiCustomAlphaUniform, GL_FLOAT, _customAlpha);
	ElementRepresentation representation = ElementRepresentation_max;
	
	const VertexArrayObject& vao = _renderingElement->vertexArrayObject();
	for (auto& i : _renderingElement->_chunks)
	{
		rs.bindTexture(0, i.layers[RenderLayer_Layer0]);
		rs.bindTexture(1, i.layers[RenderLayer_Layer1]);
		rs.setClip(true, i.clip + _customWindowOffset);
		
		if (i.representation != representation)
		{
			representation = i.representation;
			bool is3D = (i.representation == ElementRepresentation_3d);
			rs.setDepthTest(is3D);
			rs.setDepthMask(is3D);
			_guiProgram->setTransformMatrix(is3D ? _guiCamera.modelViewProjectionMatrix() : _defaultTransform);
		}

		renderer->drawElements(vao->indexBuffer(), i.first, i.count);
	}
}

void GuiRenderer::buildQuad(GuiVertexList& vertices, const GuiVertex& topLeft, const GuiVertex& topRight, 
	const GuiVertex& bottomLeft, const GuiVertex& bottomRight)
{
	vertices.fitToSize(6);
	vertices.push_back(bottomLeft);
	vertices.push_back(bottomRight);
	vertices.push_back(topRight);
	vertices.push_back(bottomLeft);
	vertices.push_back(topRight);
	vertices.push_back(topLeft);
}

void GuiRenderer::createStringVertices(GuiVertexList& vertices, const CharDescriptorList& chars,
	Alignment hAlign, Alignment vAlign, const vec2& pos, const vec4& color,
	const mat4& transform, RenderLayer layer)
{
	if (_saveFillRate)
		layer = RenderLayer_Layer0;
	
	vec4 line;
	std::vector<vec4> lines;

	for (const CharDescriptor& desc : chars)
	{
		line.w = etMax(line.w, desc.size.y);
		if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		{
			lines.push_back(line);
			line = vec4(0.0f, line.y + line.w, 0.0f, 0.0f);
		}
		else 
		{
			line.z += desc.size.x;
		}
	}
	lines.push_back(line);

	float hAlignFactor = alignmentFactor(hAlign);
	float vAlignFactor = alignmentFactor(vAlign);
	for (vec4& i : lines)
	{
		i.x -= hAlignFactor * i.z;
		i.y -= vAlignFactor * i.w;
	}
	
	size_t lineIndex = 0;
	line = lines.front();
	
	vec2 mask(layer == RenderLayer_Layer0 ? 0.0f : 1.0f, 0.0f);
	
	vertices.fitToSize(6 * chars.size());
	for (const CharDescriptor& desc : chars)
	{
		if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		{
			line = lines[++lineIndex];
		}
		else 
		{
			vec2 topLeft = line.xy() + pos;
			vec2 bottomLeft = topLeft + vec2(0.0f, desc.size.y);
			vec2 topRight = topLeft + vec2(desc.size.x, 0.0f);
			vec2 bottomRight = bottomLeft + vec2(desc.size.x, 0.0f);
			
			vec2 topLeftUV = desc.uvOrigin;
			vec2 topRightUV = topLeftUV + vec2(desc.uvSize.x, 0.0f);
			vec2 bottomLeftUV = desc.uvOrigin - vec2(0.0f, desc.uvSize.y);
			vec2 bottomRightUV = bottomLeftUV + vec2(desc.uvSize.x, 0.0f);
			vec4 charColor = desc.color * color;
			
			buildQuad(vertices,
				GuiVertex(floorv(transform * topLeft), vec4(topLeftUV, mask), charColor),
				GuiVertex(floorv(transform * topRight), vec4(topRightUV, mask), charColor),
				GuiVertex(floorv(transform * bottomLeft), vec4(bottomLeftUV, mask), charColor),
				GuiVertex(floorv(transform * bottomRight), vec4(bottomRightUV, mask), charColor));
			
			line.x += desc.size.x;
		}
	}
}

size_t GuiRenderer::measusevertexCountForImageDescriptor(const ImageDescriptor& desc)
{
	bool hasLeftSafe = desc.contentOffset.left > 0;
	bool hasTopSafe = desc.contentOffset.top > 0;
	bool hasRightSafe = desc.contentOffset.right > 0;
	bool hasBottomSafe = desc.contentOffset.bottom > 0;
	bool hasLeftTopCorner = hasLeftSafe && hasTopSafe;
	bool hasRightTopCorner = hasRightSafe && hasTopSafe;
	bool hasLeftBottomCorner = hasLeftSafe && hasBottomSafe;
	bool hasRightBottomCorner = hasRightSafe && hasBottomSafe;

	size_t numBorders = hasLeftSafe + hasTopSafe + hasRightSafe + hasBottomSafe;
	size_t numCorners = hasLeftTopCorner + hasRightTopCorner + hasLeftBottomCorner + hasRightBottomCorner;

	return 6 * (1 + numCorners + numBorders);
}

void GuiRenderer::createImageVertices(GuiVertexList& vertices, const Texture& tex, const ImageDescriptor& desc, 
	const rect& p, const vec4& color, const mat4& transform, RenderLayer layer)
{
	if (!tex.valid()) return;

	if (_saveFillRate)
		layer = RenderLayer_Layer0;
	
	bool hasLeftSafe = desc.contentOffset.left > 0;
	bool hasTopSafe = desc.contentOffset.top > 0;
	bool hasRightSafe = desc.contentOffset.right > 0;
	bool hasBottomSafe = desc.contentOffset.bottom > 0;
	bool hasLeftTopCorner = hasLeftSafe && hasTopSafe;
	bool hasRightTopCorner = hasRightSafe && hasTopSafe;
	bool hasLeftBottomCorner = hasLeftSafe && hasBottomSafe;
	bool hasRightBottomCorner = hasRightSafe && hasBottomSafe;

	size_t numBorders = hasLeftSafe + hasTopSafe + hasRightSafe + hasBottomSafe;
	size_t numCorners = hasLeftTopCorner + hasRightTopCorner + hasLeftBottomCorner + hasRightBottomCorner;

	vertices.fitToSize(6 * (1 + numCorners + numBorders));

	vec2 mask(layer == RenderLayer_Layer0 ? 0.0f : 1.0f, 0.0f);

	float width = std::abs(p.width);
	float height = std::abs(p.height);
	
	vec2 topLeft = (p.origin());
	vec2 topRight = (topLeft + vec2(width, 0.0f));
	vec2 bottomLeft = (topLeft + vec2(0.0f, height));
	vec2 bottomRight = (bottomLeft + vec2(width, 0.0f));
	vec2 centerTopLeft = (p.origin() + desc.contentOffset.origin());
	vec2 centerTopRight = (p.origin() + vec2(width - desc.contentOffset.right, desc.contentOffset.top));
	vec2 centerBottomLeft = (p.origin() + vec2(desc.contentOffset.left, height - desc.contentOffset.bottom));
	vec2 centerBottomRight = (p.origin() + vec2(width - desc.contentOffset.right, height - desc.contentOffset.bottom));
	vec2 topCenterTopLeft = (topLeft + vec2(desc.contentOffset.left, 0.0f));
	vec2 topCenterTopRight = (topLeft + vec2(width - desc.contentOffset.right, 0));
	vec2 leftCenterTopLeft = (topLeft + vec2(0, desc.contentOffset.top));
	vec2 rightCenterTopRight = (topLeft + vec2(width, desc.contentOffset.top));
	vec2 leftCenterBottomLeft = (topLeft + vec2(0, height - desc.contentOffset.bottom));
	vec2 bottomCenterBottomLeft = (topLeft + vec2(desc.contentOffset.left, height));
	vec2 bottomCenterBottomRigth = (topLeft + vec2(width - desc.contentOffset.right, height));
	vec2 rightCenterBottomRigth = (topLeft + vec2(width, height - desc.contentOffset.bottom));

	vec2 topLeftUV = tex->getTexCoord( desc.origin );
	vec2 topRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x, 0.0f) );
	vec2 bottomLeftUV = tex->getTexCoord( desc.origin + vec2(0.0f, desc.size.y) );
	vec2 bottomRightUV = tex->getTexCoord( desc.origin + desc.size );
	vec2 centerTopLeftUV = tex->getTexCoord( desc.centerPartTopLeft() );
	vec2 centerBottomLeftUV = tex->getTexCoord( desc.centerPartBottomLeft() );
	vec2 centerTopRightUV = tex->getTexCoord( desc.centerPartTopRight() );
	vec2 centerBottomRightUV = tex->getTexCoord( desc.centerPartBottomRight() );
	vec2 topCenterTopLeftUV = tex->getTexCoord( desc.origin + vec2(desc.contentOffset.left, 0) );
	vec2 topCenterTopRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x - desc.contentOffset.right, 0) );
	vec2 leftCenterTopLeftUV = tex->getTexCoord( desc.origin + vec2(0, desc.contentOffset.top) );
	vec2 rightCenterTopRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x, desc.contentOffset.top) );
	vec2 leftCenterBottomLeftUV = tex->getTexCoord( desc.origin + vec2(0, desc.size.y - desc.contentOffset.bottom) );
	vec2 bottomCenterBottomLeftUV = tex->getTexCoord( desc.origin + vec2(desc.contentOffset.left, desc.size.y) );
	vec2 bottomCenterBottomRigthUV = tex->getTexCoord( desc.origin + vec2(desc.size.x - desc.contentOffset.right, desc.size.y) );
	vec2 rightCenterBottomRigthUV = tex->getTexCoord( desc.origin + vec2( desc.size.x, desc.size.y - desc.contentOffset.bottom));

	buildQuad(vertices, 
		GuiVertex(transform * centerTopLeft, vec4(centerTopLeftUV, mask), color ), 
		GuiVertex(transform * centerTopRight, vec4(centerTopRightUV, mask), color ),
		GuiVertex(transform * centerBottomLeft, vec4(centerBottomLeftUV, mask), color ),
		GuiVertex(transform * centerBottomRight, vec4(centerBottomRightUV, mask), color ) );

	if (hasLeftTopCorner)
	{
		buildQuad(vertices, 
			GuiVertex(transform * topLeft, vec4(topLeftUV, mask), color), 
			GuiVertex(transform * topCenterTopLeft, vec4(topCenterTopLeftUV, mask), color), 
			GuiVertex(transform * leftCenterTopLeft, vec4(leftCenterTopLeftUV, mask), color), 
			GuiVertex(transform * centerTopLeft, vec4(centerTopLeftUV, mask), color) );
	}

	if (hasRightTopCorner)
	{
		buildQuad(vertices,
			GuiVertex(transform * topCenterTopRight, vec4(topCenterTopRightUV, mask), color),
			GuiVertex(transform * topRight, vec4(topRightUV, mask), color), 
			GuiVertex(transform * centerTopRight, vec4(centerTopRightUV, mask), color), 
			GuiVertex(transform * rightCenterTopRight, vec4(rightCenterTopRightUV, mask), color) );
	}

	if (hasLeftBottomCorner)
	{
		buildQuad(vertices, 
			GuiVertex(transform * leftCenterBottomLeft, vec4(leftCenterBottomLeftUV, mask), color), 
			GuiVertex(transform * centerBottomLeft, vec4(centerBottomLeftUV, mask), color), 
			GuiVertex(transform * bottomLeft, vec4(bottomLeftUV, mask), color), 
			GuiVertex(transform * bottomCenterBottomLeft, vec4(bottomCenterBottomLeftUV, mask), color) );
	}

	if (hasRightBottomCorner)
	{
		buildQuad(vertices, 
			GuiVertex(transform * centerBottomRight, vec4(centerBottomRightUV, mask), color), 
			GuiVertex(transform * rightCenterBottomRigth, vec4(rightCenterBottomRigthUV, mask), color), 
			GuiVertex(transform * bottomCenterBottomRigth, vec4(bottomCenterBottomRigthUV, mask), color), 
			GuiVertex(transform * bottomRight, vec4(bottomRightUV, mask), color) );
	}

	if (hasTopSafe)
	{
		vec2 tl = hasLeftTopCorner ? topCenterTopLeft : topLeft;
		vec2 tr = hasRightTopCorner ? topCenterTopRight : topRight;
		vec2 bl = hasLeftTopCorner ? centerTopLeft : leftCenterTopLeft;
		vec2 br = hasRightTopCorner ? centerTopRight : rightCenterTopRight;
		vec2 tlUV = hasLeftTopCorner ? topCenterTopLeftUV : topLeftUV;
		vec2 trUV = hasRightTopCorner ? topCenterTopRightUV : topRightUV;
		vec2 blUV = hasLeftTopCorner ? centerTopLeftUV : leftCenterTopLeftUV;
		vec2 brUV = hasRightTopCorner ? centerTopRightUV : rightCenterTopRightUV;

		buildQuad(vertices, 
			GuiVertex(transform * tl, vec4(tlUV, mask), color),
			GuiVertex(transform * tr, vec4(trUV, mask), color),
			GuiVertex(transform * bl, vec4(blUV, mask), color), 
			GuiVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasLeftSafe)
	{
		vec2 tl = hasLeftTopCorner ? leftCenterTopLeft : topLeft;
		vec2 tr = hasLeftTopCorner ? centerTopLeft : topCenterTopLeft;
		vec2 bl = hasLeftBottomCorner ? leftCenterBottomLeft : bottomLeft;
		vec2 br = hasLeftBottomCorner ? centerBottomLeft : bottomCenterBottomLeft;
		vec2 tlUV = hasLeftTopCorner ? leftCenterTopLeftUV : topLeftUV;
		vec2 trUV = hasLeftTopCorner ? centerTopLeftUV : topCenterTopLeftUV;
		vec2 blUV = hasLeftBottomCorner ? leftCenterBottomLeftUV : bottomLeftUV;
		vec2 brUV = hasLeftBottomCorner ? centerBottomLeftUV : bottomCenterBottomLeftUV;

		buildQuad(vertices,
			GuiVertex(transform * tl, vec4(tlUV, mask), color), 
			GuiVertex(transform * tr, vec4(trUV, mask), color),
			GuiVertex(transform * bl, vec4(blUV, mask), color),
			GuiVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasBottomSafe)
	{
		vec2 tl = hasLeftBottomCorner ? centerBottomLeft : leftCenterBottomLeft;
		vec2 tr = hasRightBottomCorner ? centerBottomRight : rightCenterBottomRigth;
		vec2 bl = hasLeftBottomCorner ? bottomCenterBottomLeft : bottomLeft;
		vec2 br = hasRightBottomCorner ? bottomCenterBottomRigth : bottomRight;
		vec2 tlUV = hasLeftBottomCorner ? centerBottomLeftUV : leftCenterBottomLeftUV;
		vec2 trUV = hasRightBottomCorner ? centerBottomRightUV : rightCenterBottomRigthUV;
		vec2 blUV = hasLeftBottomCorner ? bottomCenterBottomLeftUV : bottomLeftUV;
		vec2 brUV = hasRightBottomCorner ? bottomCenterBottomRigthUV : bottomRightUV;

		buildQuad(vertices,
			GuiVertex(transform * tl, vec4(tlUV, mask), color), 
			GuiVertex(transform * tr, vec4(trUV, mask), color), 
			GuiVertex(transform * bl, vec4(blUV, mask), color), 
			GuiVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasRightSafe)
	{
		vec2 tl = hasRightTopCorner ? centerTopRight : topCenterTopRight;
		vec2 tr = hasRightTopCorner ? rightCenterTopRight : topRight;
		vec2 bl = hasRightBottomCorner ? centerBottomRight : bottomCenterBottomRigth;
		vec2 br = hasRightBottomCorner ? rightCenterBottomRigth : bottomRight;
		vec2 tlUV = hasRightTopCorner ? centerTopRightUV : topCenterTopRightUV;
		vec2 trUV = hasRightTopCorner ? rightCenterTopRightUV : topRightUV;
		vec2 blUV = hasRightBottomCorner ? centerBottomRightUV : bottomCenterBottomRigthUV;
		vec2 brUV = hasRightBottomCorner ? rightCenterBottomRigthUV : bottomRightUV;

		buildQuad(vertices, 
			GuiVertex(transform * tl, vec4(tlUV, mask), color),
			GuiVertex(transform * tr, vec4(trUV, mask), color),
			GuiVertex(transform * bl, vec4(blUV, mask), color), 
			GuiVertex(transform * br, vec4(brUV, mask), color) );
	}
}

void GuiRenderer::createColorVertices(GuiVertexList& vertices, const rect& p, const vec4& color, 
	const mat4& transform)
{
	vec2 topLeft = p.origin();
	vec2 topRight = topLeft + vec2(p.width, 0.0f);
	vec2 bottomLeft = topLeft + vec2(0.0f, p.height);
	vec2 bottomRight = bottomLeft + vec2(p.width, 0.0f);
	
	vec4 texCoord(0.0f, 0.0f, 0.0f, 1.0f);
	
	buildQuad(vertices, GuiVertex(transform * topLeft, texCoord, color),
		GuiVertex(transform * topRight, texCoord, color), GuiVertex(transform * bottomLeft, texCoord, color),
		GuiVertex(transform * bottomRight, texCoord, color));
}

void GuiRenderer::setCustomOffset(const vec2& offset)
{
	_customOffset = 2.0f * offset;
	_customWindowOffset.left = static_cast<int>(offset.x * _rc->size().x);
	_customWindowOffset.top = static_cast<int>(offset.y * _rc->size().y);
}

#define COMMON_VERETX_SHADER_UNIFORMS	\
	"uniform mat4 mTransform;"\
	"uniform vec2 vCustomOffset;"\
	"uniform float customAlpha;"\
	"etVertexIn vec3 Vertex;"\
	"etVertexIn vec4 TexCoord0;"\
	"etVertexIn vec4 Color;"\
	"etVertexOut vec2 vTexCoord;"\
	"etVertexOut vec4 tintColor;"\
	"etVertexOut float colorMask;"

#define COMMON_VERTEX_SHADER_CODE 	\
	"vTexCoord = TexCoord0.xy;"\
	"colorMask = TexCoord0.w;"\
	"tintColor = Color * vec4(1.0, 1.0, 1.0, customAlpha);"\
	"vec4 vTransformed = mTransform * vec4(Vertex, 1.0);"\
	"gl_Position = vTransformed + vec4(vTransformed.w * vCustomOffset, 0.0, 0.0);"

#define COMMON_FRAGMENT_SHADER_UNIFORMS	\
	"uniform etLowp sampler2D layer0_texture;"\
	"etFragmentIn etMediump vec2 vTexCoord;"\
	"etFragmentIn etLowp vec4 tintColor;"\
	"etFragmentIn etLowp float colorMask;"\

std::string gui_default_vertex_src =
	COMMON_VERETX_SHADER_UNIFORMS
	"etVertexOut float texturesMask;"\
	"void main()"
	"{"
	"	texturesMask = TexCoord0.z;"
		COMMON_VERTEX_SHADER_CODE
	"}";

std::string gui_savefillrate_vertex_src =
	COMMON_VERETX_SHADER_UNIFORMS
	"void main()"
	"{"
		COMMON_VERTEX_SHADER_CODE
	"}";

std::string gui_default_frag_src =
	COMMON_FRAGMENT_SHADER_UNIFORMS
	"uniform etLowp sampler2D layer1_texture;"
	"etFragmentIn etLowp float texturesMask;"
	"void main()"
	"{"
	"	etFragmentOut = mix(mix(etTexture2D(layer0_texture, vTexCoord), "
	"		etTexture2D(layer1_texture, vTexCoord), texturesMask), vec4(1.0), colorMask) * tintColor;"
	"}";

std::string gui_savefillrate_frag_src = 
	COMMON_FRAGMENT_SHADER_UNIFORMS
	"void main()"
	"{"
	"	etFragmentOut = mix(etTexture2D(layer0_texture, vTexCoord), vec4(1.0), colorMask) * tintColor;"
	"}";
