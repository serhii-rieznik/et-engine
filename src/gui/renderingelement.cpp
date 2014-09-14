/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/gui/renderingelement.h>

using namespace et;
using namespace et::gui;

RenderingElement::RenderingElement(RenderContext* rc) : _rs(rc->renderState()),
	_indexArray(new IndexArray(IndexArrayFormat_16bit, 0, PrimitiveType_Triangles)), _changed(false)
{
	VertexDeclaration decl(true, Usage_Position, Type_Vec3);
	decl.push_back(Usage_TexCoord0, Type_Vec4);
	decl.push_back(Usage_Color, Type_Vec4);
	
	std::string nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao";

	_vao = rc->vertexBufferFactory().createVertexArrayObject(nameId,
		VertexArray::Pointer(new VertexArray(decl, true)), BufferDrawType_Stream,
		_indexArray, BufferDrawType_Static);
}

void RenderingElement::clear()
{
	_vertexList.setOffset(0);
	_indexArray->setActualSize(0);
	_chunks.clear();
	_changed = true;
}

const VertexArrayObject& RenderingElement::vertexArrayObject()
{
	_rs.bindVertexArray(_vao);

	if (_changed)
	{
		size_t count = _vertexList.lastElementIndex();
		_indexArray->setActualSize(count);
		_vao->vertexBuffer()->setData(_vertexList.data(), count * _vertexList.typeSize());
		_vao->indexBuffer()->setData(_indexArray);
		_changed = false;
	}

	return _vao;
}
