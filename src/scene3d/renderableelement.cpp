/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/renderableelement.h>

using namespace et;
using namespace et::s3d;

RenderableElement::RenderableElement(const std::string& name, BaseElement* parent) :
	ElementContainer(name, parent)
{
	setFlag(Flag_Renderable);
}

RenderableElement::RenderableElement(const std::string& name, const SceneMaterial::Pointer& mat,
	BaseElement* parent) : ElementContainer(name, parent), _material(mat)
{
	setFlag(Flag_Renderable);
}

void RenderableElement::serialize(Dictionary stream, const std::string& basePath)
{
	stream.setStringForKey(kMaterialName, material()->name());
	ElementContainer::serialize(stream, basePath);
}

void RenderableElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	auto materialName = stream.stringForKey(kMaterialName)->content;
	setMaterial(helper->sceneMaterialWithName(materialName));
	ElementContainer::deserialize(stream, helper);
}

void RenderableElement::addRenderBatch(RenderBatch::Pointer rb)
{
	_renderBatches.push_back(rb);
}

void RenderableElement::prepareRenderBatches()
{
	for (auto& rb : _renderBatches)
	{
		rb->setTransformation(finalTransform());
	}
}

std::vector<RenderBatch::Pointer>& RenderableElement::renderBatches()
{
	return _renderBatches;
}

const std::vector<RenderBatch::Pointer>& RenderableElement::renderBatches() const
{
	return _renderBatches;
}
