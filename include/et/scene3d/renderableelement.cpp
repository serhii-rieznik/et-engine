/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/renderableelement.h>

namespace et 
{

namespace s3d
{

RenderableElement::RenderableElement(const std::string& name, BaseElement* parent) :
	ElementContainer(name, parent)
{
}

void RenderableElement::addRenderBatch(RenderBatch::Pointer rb)
{
	_renderBatches.push_back(rb);
}

Vector<RenderBatch::Pointer>& RenderableElement::renderBatches()
{
	return _renderBatches;
}

const Vector<RenderBatch::Pointer>& RenderableElement::renderBatches() const
{
	return _renderBatches;
}

void RenderableElement::setMaterial(const Material::Pointer& mtl)
{
	for (RenderBatch::Pointer& rb : renderBatches())
	{
		rb->setMaterial(mtl);
	}
}

}
}
