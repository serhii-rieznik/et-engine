/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/conversion.h>
#include <et/app/application.h>
#include <et/scene3d/baseelement.h>

using namespace et;
using namespace et::s3d;

BaseElement::BaseElement(const std::string& name, BaseElement* parent) :
	ElementHierarchy(parent)
{
	setName(name);
	
	_animationTimer.expired.connect([this](NotifyTimer* timer)
	{
		ET_ASSERT(!_animations.empty());
		
		const auto& a = _animations.front();
		float dt = a.startTime() + (timer->actualTime() - timer->startTime());
		if ((a.outOfRangeMode() == Animation::OutOfRangeMode_Once) && (dt > a.stopTime()))
		{
			dt = a.startTime();
			timer->cancelUpdates();
		}
		setAnimationTime(dt);
	});
}

void BaseElement::setParent(BaseElement* p)
{
	invalidateTransform();
	ElementHierarchy::setParent(p);
}

void BaseElement::invalidateTransform()
{
	ComponentTransformable::invalidateTransform();
	
	for (auto i : children())
		i->invalidateTransform();
	
	transformInvalidated();
}

void BaseElement::buildTransform()
{
	_cachedFinalTransform = localTransform();
	
	if (parent() != nullptr)
	{
		_cachedFinalTransform *= parent()->finalTransform();
	}
	
	_cachedFinalInverseTransform = _cachedFinalTransform.inverse();
}

const mat4& BaseElement::localTransform()
{
	_cachedLocalTransform = _animations.empty() ? transform() : _animationTransform;
	return _cachedLocalTransform;
}

const mat4& BaseElement::finalTransform()
{
	if (!transformValid())
	{
		buildTransform();
	}
	return _cachedFinalTransform;
}

const mat4& BaseElement::finalInverseTransform()
{
	if (!transformValid())
		buildTransform();
	
	return _cachedFinalInverseTransform;
}

bool BaseElement::isKindOf(ElementType t) const
{
	return (t == ElementType::DontCare) || (type() == t);
}

BaseElement::Pointer BaseElement::childWithName(const std::string& name, ElementType ofType, bool assertFail)
{
	for (const BaseElement::Pointer& i : children())
	{
		BaseElement::Pointer element = childWithNameCallback(name, i, ofType);
		if (element.valid())
			return element;
	}

	if (assertFail)
		ET_FAIL_FMT("Unable to find child: %s", name.c_str());

	return BaseElement::Pointer();
}

BaseElement::List BaseElement::childrenOfType(ElementType ofType) const
{
	BaseElement::List list;
	
	for (const BaseElement::Pointer& i : children())
		childrenOfTypeCallback(ofType, list, i);
	
	return list;
}

BaseElement::List BaseElement::childrenHavingFlag(size_t flag) const
{
	BaseElement::List list;
	
	for (const BaseElement::Pointer& i : children())
		childrenHavingFlagCallback(flag, list, i);
	
	return list;
}

BaseElement::Pointer BaseElement::childWithNameCallback(const std::string& name, BaseElement::Pointer root, ElementType ofType)
{
	if (root->isKindOf(ofType) && (root->name() == name)) return root;

	for (const auto& i : root->children())
	{
		BaseElement::Pointer element = childWithNameCallback(name, i, ofType);
		if (element.valid() && element->isKindOf(ofType))
			return element;
	}

	return BaseElement::Pointer();
}

void BaseElement::childrenOfTypeCallback(ElementType t, BaseElement::List& list, BaseElement::Pointer root) const
{
	if (root->isKindOf(t))
		list.push_back(root);

	for (const auto& i : root->children())
		childrenOfTypeCallback(t, list, i);
}

void BaseElement::childrenHavingFlagCallback(size_t flag, BaseElement::List& list, BaseElement::Pointer root) const
{
	if (root->hasFlag(flag))
		list.push_back(root);

	for (const auto& i : root->children())
		childrenHavingFlagCallback(flag, list, i);
}

void BaseElement::clear()
{
	removeChildren();
}

void BaseElement::clearRecursively()
{
	for (auto& c : children())
		c->clearRecursively();
	
	clear();
}

void BaseElement::serialize(Dictionary stream, const std::string& basePath)
{
	serializeGeneralParameters(stream);
	serializeChildren(stream, basePath);
}

void BaseElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	deserializeGeneralParameters(stream);
	deserializeChildren(stream, helper);
}

void BaseElement::serializeGeneralParameters(Dictionary stream)
{
	stream.setStringForKey(kName, name());
	stream.setIntegerForKey(kElementTypeCode, static_cast<int64_t>(type()));
	stream.setIntegerForKey(kFlagsValue, flags());
	stream.setArrayForKey(kTranslation, vec3ToArray(translation()));
	stream.setArrayForKey(kScale, vec3ToArray(scale()));
	stream.setArrayForKey(kOrientation, quaternionToArray(orientation()));
	stream.setArrayForKey(kAdditionalTransform, matrixToArray(additionalTransform()));

	if (!_properites.empty())
	{
		ArrayValue propertiesArray;
		propertiesArray->content.reserve(_properites.size());
		for (const auto& i : _properites)
			propertiesArray->content.push_back(StringValue(i));
		stream.setArrayForKey(kProperties, propertiesArray);
	}

	if (!_animations.empty())
	{
		ArrayValue animationsArray;
		animationsArray->content.reserve(_animations.size());
		for (const auto& a : _animations)
		{
			Dictionary animationDictionary;
			a.serialize(animationDictionary);
			animationsArray->content.push_back(animationDictionary);
		}
		stream.setArrayForKey(kAnimations, animationsArray);
	}
}

void BaseElement::deserializeGeneralParameters(Dictionary stream)
{
	setName(stream.stringForKey(kName)->content);

	auto typeCode = stream.integerForKey(kElementTypeCode)->content;
	bool isValidType = (typeCode >= 0) && (typeCode < ElementType_Max) && (typeCode == static_cast<int64_t>(type()));

	if (!isValidType)
	{
		log::warning("Deserializing element %s with invalid type code %llu, supposed to be: %llu",
			name().c_str(), typeCode, uint64_t(type()));
		return;
	}
	
	setFlags(stream.integerForKey(kFlagsValue)->content);
	setTranslation(arrayToVec3(stream.arrayForKey(kTranslation)));
	setScale(arrayToVec3(stream.arrayForKey(kScale)));
	setOrientation(arrayToQuaternion(stream.arrayForKey(kOrientation)));
	setAdditionalTransform(arrayToMatrix(stream.arrayForKey(kAdditionalTransform)));
	
	auto propsArray = stream.arrayForKey(kProperties);
	for (StringValue prop : propsArray->content)
		_properites.insert(prop->content);
	
	auto animsArray = stream.arrayForKey(kAnimations);
	for (Dictionary anim : animsArray->content)
		_animations.emplace_back(anim);
}

void BaseElement::serializeChildren(Dictionary stream, const std::string& basePath)
{
	if (children().empty()) return;

	ArrayValue childrenArray;
	childrenArray->content.reserve(children().size());

	for (auto i :children())
	{
		Dictionary object;
		i->serialize(object, basePath);
		childrenArray->content.push_back(object);
	}

	stream.setArrayForKey(kChildren, childrenArray);
}

void BaseElement::deserializeChildren(Dictionary stream, SerializationHelper* helper)
{
	ArrayValue childrenArray = stream.arrayForKey(kChildren);
	for (Dictionary object : childrenArray->content)
	{
		auto elementTypeCode = object.integerForKey(kElementTypeCode)->content;
		if ((elementTypeCode >= ElementType_First) && (elementTypeCode < ElementType_Max))
		{
			auto child = helper->createElementOfType(static_cast<ElementType>(elementTypeCode), this);
			child->deserialize(object, helper);
		}
	}
}

void BaseElement::duplicateChildrenToObject(BaseElement* object)
{
	auto allChildren = children();
	for (auto& c : allChildren)
	{
		c->duplicate()->setParent(object);
	}
}

void BaseElement::duplicateBasePropertiesToObject(BaseElement* object)
{
	object->setFlags(flags());
	object->setScale(scale());
	object->setTranslation(translation());
	object->setOrientation(orientation());
	object->setAdditionalTransform(additionalTransform());
	
	for (const auto& p : properties())
	{
		object->addPropertyString(p);
	}
}

bool BaseElement::hasPropertyString(const std::string& s) const
{
	return _properites.find(s) != _properites.end();
}

void BaseElement::addAnimation(const Animation& a)
{
	_animations.push_back(a);
	_animationTransform = a.transformation(a.startTime());
}

void BaseElement::removeAnimations()
{
	_animations.clear();
	invalidateTransform();
}

void BaseElement::animate()
{
	if (_animations.empty()) return;
	
	_animationTimer.start(currentTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}

void BaseElement::stopAnimation()
{
	_animationTimer.cancelUpdates();
}

void BaseElement::animateRecursive()
{
	animate();

	for (auto c : children())
		c->animateRecursive();
}

void BaseElement::stopAnimationRecursive()
{
	stopAnimation();

	for (auto c : children())
		c->stopAnimationRecursive();
}

bool BaseElement::animating() const 
{
	return _animationTimer.running();
}

bool BaseElement::anyChildAnimating() const
{
	if (animating()) 
		return true;

	for (auto c : children())
	{
		if (c->anyChildAnimating())
			return true;
	}

	return false;
}

Animation& BaseElement::defaultAnimation()
{
	return _animations.empty() ? _emptyAnimation : _animations.front();
}

const Animation& BaseElement::defaultAnimation() const
{
	return _animations.empty() ? _emptyAnimation : _animations.front();
}

void BaseElement::setAnimationTime(float t)
{
	_animationTransform = _animations.empty() ? identityMatrix : _animations.front().transformation(t);
	invalidateTransform();
}

void BaseElement::setAnimationTimeRecursive(float a)
{
	setAnimationTime(a);
	for (auto c : children())
		c->setAnimationTimeRecursive(a);
}
