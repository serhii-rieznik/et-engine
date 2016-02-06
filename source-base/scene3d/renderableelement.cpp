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
	
	ArrayValue batches;
	batches->content.reserve(renderBatches().size());
	for (const auto& rb : renderBatches())
	{
		batches->content.push_back(rb->serialize());
	}
	stream.setArrayForKey(kRenderBatches, batches);
	
	ElementContainer::serialize(stream, basePath);
}

void RenderableElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	auto batches = stream.arrayForKey(kRenderBatches);
	for (Dictionary rb : batches->content)
	{
		uint32_t startIndex = static_cast<uint32_t>(rb.integerForKey(kStartIndex)->content);
		uint32_t numIndexes = static_cast<uint32_t>(rb.integerForKey(kIndexesCount)->content);
		auto storageName = rb.stringForKey(kVertexStorageName)->content;
		auto indexName = rb.stringForKey(kIndexArrayName)->content;
		auto materialName = rb.stringForKey(kMaterialName)->content;
		
		auto mat = helper->materialWithName(materialName);
		auto vao = helper->vertexArrayWithStorageName(storageName);
		auto vs = helper->vertexStorageWithName(storageName);
		auto ia = helper->indexArrayWithName(indexName);

		auto batch = RenderBatch::Pointer::create(mat, vao, identityMatrix, startIndex, numIndexes);
		batch->setVertexStorage(vs);
		batch->setIndexArray(ia);
		batch->calculateBoundingBox();
		
		addRenderBatch(batch);
	}

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
		rb->setTransformation(additionalTransform() * finalTransform());
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

const mat4& RenderableElement::finalInverseTransform()
{
	return ElementContainer::finalInverseTransform();
}

const mat4& RenderableElement::finalTransform()
{
	return ElementContainer::finalTransform();
}
