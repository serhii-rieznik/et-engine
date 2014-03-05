/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/scene3d/baseelement.h>

using namespace et;
using namespace et::s3d;

Element::Element(const std::string& name, Element* parent) :
	ElementHierarchy(parent),  tag(0), _animationTransform(identityMatrix), _active(true)
{
	setName(name);
	
	_animationTimer.expired.connect([this](NotifyTimer* timer)
	{
		ET_ASSERT(!_animations.empty());
		
		float dt = timer->actualTime() - timer->startTime();
		_animationTransform = _animations.front().transformation(dt);
		
		invalidateTransform();
	});
}

void Element::setParent(Element* p)
{
	invalidateTransform();
	ElementHierarchy::setParent(p);
}

void Element::invalidateTransform()
{
	ComponentTransformable::invalidateTransform();
	
	for (auto i : children())
		i->invalidateTransform();
}

void Element::buildTransform()
{
	_cachedFinalTransform = _animations.empty() ? transform() : _animationTransform;
	
	if (parent() != nullptr)
		_cachedFinalTransform *= parent()->finalTransform();
	
	_cachedFinalInverseTransform = _cachedFinalTransform.inverse();
}

const mat4& Element::finalTransform()
{
	if (!transformValid())
		buildTransform();
	
	return _cachedFinalTransform;
}

const mat4& Element::finalInverseTransform()
{
	if (!transformValid())
		buildTransform();
	
	return _cachedFinalInverseTransform;
}

bool Element::isKindOf(ElementType t) const
{
	return (t == ElementType_Any) || (type() == t);
}

Element::Pointer Element::childWithName(const std::string& name, ElementType ofType, bool assertFail)
{
	for (const Element::Pointer& i : children())
	{
		Element::Pointer element = childWithNameCallback(name, i, ofType);
		if (element.valid())
			return element;
	}

	if (assertFail)
	{
		log::error("Unable to find child with name: %s", name.c_str());
		ET_ASSERT("Unable to find child" && 0);
	}

	return Element::Pointer();
}

Element::List Element::childrenOfType(ElementType ofType) const
{
	Element::List list;
	
	for (const Element::Pointer& i : children())
		childrenOfTypeCallback(ofType, list, i);
	
	return list;
}

Element::List Element::childrenHavingFlag(size_t flag)
{
	Element::List list;
	
	for (const Element::Pointer& i : children())
		childrenHavingFlagCallback(flag, list, i);
	
	return list;
}

Element::Pointer Element::childWithNameCallback(const std::string& name, Element::Pointer root, ElementType ofType)
{
	if (root->isKindOf(ofType) && (root->name() == name)) return root;

	for (const auto& i : root->children())
	{
		Element::Pointer element = childWithNameCallback(name, i, ofType);
		if (element.valid() && element->isKindOf(ofType))
			return element;
	}

	return Element::Pointer();
}

void Element::childrenOfTypeCallback(ElementType t, Element::List& list, Element::Pointer root) const
{
	if (root->isKindOf(t))
		list.push_back(root);

	for (const auto& i : root->children())
		childrenOfTypeCallback(t, list, i);
}

void Element::childrenHavingFlagCallback(size_t flag, Element::List& list, Element::Pointer root)
{
	if (root->hasFlag(flag))
		list.push_back(root);

	for (const auto& i : root->children())
		childrenHavingFlagCallback(flag, list, i);
}

void Element::clear()
{
	removeChildren();
}

void Element::serializeGeneralParameters(std::ostream& stream, SceneVersion version)
{
	serializeString(stream, name());
	serializeInt(stream, _active);
	serializeInt(stream, flags());
	serializeVector(stream, translation());
	serializeVector(stream, scale());
	serializeQuaternion(stream, orientation());
	
	if (version >= SceneVersion_1_0_1)
	{
		serializeInt(stream, _properites.size());
		for (const auto& i : _properites)
			serializeString(stream, i);
	}
	
	if (version >= SceneVersion_1_0_4)
	{
		serializeInt(stream, _animations.size());
		for (const auto& a : _animations)
			a.serialize(stream);
	}
}

void Element::deserializeGeneralParameters(std::istream& stream, SceneVersion version)
{
	setName(deserializeString(stream));
	
	_active = deserializeInt(stream) != 0;
	setFlags(deserializeUInt(stream));
	setTranslation(deserializeVector<vec3>(stream));
	setScale(deserializeVector<vec3>(stream));
	setOrientation(deserializeQuaternion(stream));

	if (version >= SceneVersion_1_0_1)
	{
		size_t numProperties = deserializeUInt(stream);
		for (size_t i = 0; i < numProperties; ++i)
			_properites.insert(deserializeString(stream));
	}
	
	if (version >= SceneVersion_1_0_4)
	{
		size_t numAnimations = deserializeUInt(stream);
		_animations.reserve(numAnimations);
		
		for (size_t i = 0; i < numAnimations; ++i)
			addAnimation(Animation(stream));
		
		animate();
	}
}

void Element::serializeChildren(std::ostream& stream, SceneVersion version)
{
	serializeInt(stream, children().size());
	for (auto& i :children())
	{
		serializeInt(stream, i->type());
		i->serialize(stream, version);
	}
}

void Element::deserializeChildren(std::istream& stream, ElementFactory* factory, SceneVersion version)
{
	size_t numChildren = deserializeUInt(stream);
	for (size_t i = 0; i < numChildren; ++i)
	{
		size_t type = deserializeUInt(stream);
		Element::Pointer child = factory->createElementOfType(type, (type == ElementType_Storage) ? 0 : this);
		child->deserialize(stream, factory, version);
	}
}

void Element::duplicateChildrenToObject(Element* object)
{
	for (auto& c : children())
		c->duplicate()->setParent(object);
}

void Element::duplicateBasePropertiesToObject(Element* object)
{
	object->setScale(scale());
	object->setTranslation(translation());
	object->setOrientation(orientation());
	
	for (const auto& p : properties())
		object->addPropertyString(p);
	
	object->tag = tag;
}

void Element::serialize(std::ostream&, SceneVersion)
{
	log::error("Serialization method was not overloaded");
#if (ET_DEBUG)
	abort();
#endif
}

void Element::deserialize(std::istream&, ElementFactory*, SceneVersion)
{
	log::error("Deserialization method was not overloaded");
#if (ET_DEBUG)
	abort();
#endif
}

void Element::setActive(bool active)
{
	_active = active;
}

bool Element::hasPropertyString(const std::string& s) const
{
	return _properites.find(s) != _properites.end();
}

void Element::addAnimation(const Animation& a)
{
	_animations.push_back(a);
	setFlag(Flag_HasAnimations);
}

void Element::animate()
{
	if (_animations.empty()) return;
	
	_animationTimer.start(mainTimerPool(), 0.0f, NotifyTimer::RepeatForever);
}
