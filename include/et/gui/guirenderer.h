/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <stack>
#include <et/core/containers.h>
#include <et/core/objectscache.h>
#include <et/camera/camera.h>
#include <et/gui/font.h>
#include <et/gui/renderingelement.h>
#include <et/apiobjects/program.h>

namespace et
{
	namespace gui
	{
		class GuiRenderer
		{
		public:
			GuiRenderer(RenderContext* rc, bool saveFillRate);

			void beginRender(RenderContext* rc);
			void render(RenderContext* rc);
			void endRender(RenderContext* rc);

			void resetClipRect();
			void pushClipRect(const recti&);
			void popClipRect();

			void setProjectionMatrices(const vec2& contextSize);
			void setRendernigElement(const RenderingElement::Pointer& r);

			size_t addVertices(const GuiVertexList& vertices, const Texture& texture,
				ElementRepresentation cls, RenderLayer layer);

			size_t measusevertexCountForImageDescriptor(const ImageDescriptor& desc);
			
			void createStringVertices(GuiVertexList& vertices, const CharDescriptorList& chars, 
				Alignment hAlign, Alignment vAlign, const vec2& pos,
				const vec4& color, const mat4& transform, RenderLayer layer);

			void createImageVertices(GuiVertexList& vertices, const Texture& tex, const ImageDescriptor& desc, 
				const rect& p, const vec4& color, const mat4& transform, RenderLayer layer);

			void createColorVertices(GuiVertexList& vertices, const rect& p, const vec4& color,
				const mat4& transform);
			
			void buildQuad(GuiVertexList& vertices, const GuiVertex& topLeft, const GuiVertex& topRight,
				const GuiVertex& bottomLeft, const GuiVertex& bottomRight);
			
			void setCustomOffset(const vec2& offset);
			
			void setCustomAlpha(float alpha)
				{ _customAlpha = alpha; }
			
			const Camera& camera3d() const 
				{ return _guiCamera; }

		private:
			void init(RenderContext* rc);
			void alloc(size_t count);
			GuiVertexPointer allocateVertices(size_t count, const Texture& texture,
				ElementRepresentation cls, RenderLayer layer);

			GuiRenderer& operator = (const GuiRenderer&)
				{ return *this; }
			
		private:
			RenderContext* _rc;
			ObjectsCache _sharedCache;
			RenderingElement::Pointer _renderingElement;
			Texture _lastTextures[RenderLayer_max];
			Texture _defaultTexture;
			
			Program::Pointer _guiProgram;
			mat4 _defaultTransform;
			int _guiCustomOffsetUniform;
			int _guiCustomAlphaUniform;
			
			Camera _guiCamera;

			std::stack<recti> _clip;
			vec2 _customOffset;
			recti _customWindowOffset;
			float _customAlpha;
	
			size_t _blendState;
			bool _saveFillRate;
			bool _depthTestEnabled;
			bool _depthMask;
			bool _blendEnabled;
		};
	}
}
