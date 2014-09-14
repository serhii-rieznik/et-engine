/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexarrayobject.h>
#include <et/gui/guibase.h>

namespace et
{
	namespace gui
	{
		class RenderingElement : public Shared
		{
		public:
			typedef IntrusivePtr<RenderingElement> Pointer;

		public:
			RenderingElement(RenderContext* rc);
			void clear();

			const VertexArrayObject& vertexArrayObject();

		private:
			friend class GuiRenderer;

		private:
			RenderState& _rs;
			RenderChunkList _chunks;
			IndexArray::Pointer _indexArray;
			GuiVertexList _vertexList;

			VertexArrayObject _vao;

			bool _changed;
		};
	}
}
