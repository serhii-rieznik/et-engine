/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/scene3d/element.h>

using namespace et;
using namespace et::s3d;

Element::Element(const std::string& name, Element* parent) :
	ElementHierarchy(parent),  tag(0), _active(true)
{
	setName(name);
}

void Element::setParent(Element* p)
{
	invalidateTransform();
	ElementHierarchy::setParent(p);
}

void Element::invalidateTransform()
{
	ComponentTransformable::invalidateTransform();
	ET_ITERATE(children(), Element::Pointer&, i,
	{
		i->invalidateTransform();
	});
}

mat4 Element::finalTransform()
{
	if (!transformValid())
		_cachedFinalTransform = parent() ?  transform() * parent()->finalTransform() : transform();

	return _cachedFinalTransform;
}

bool Element::isKindOf(ElementType t) const
{
	return (t == ElementType_Any) || (type() == t);
}

Element::Pointer Element::childWithName(const std::string& name, ElementType ofType, bool assertFail)
{
	ET_ITERATE(children(), const Element::Pointer&, i, 
	{
		Element::Pointer element = childWithNameCallback(name, i, ofType);
		if (element.valid())
			return element;
	})

	if (assertFail)
	{
		log::error("Unable to find child with name: %s", name.c_str());
		assert("Unable to find child" && 0);
	}

	return Element::Pointer();
}

Element::List Element::childrenOfType(ElementType ofType) const
{
	Element::List list;
	ET_ITERATE(children(), const Element::Pointer&, i, childrenOfTypeCallback(ofType, list, i))
	return list;
}

Element::List Element::childrenHavingFlag(size_t flag)
{
	Element::List list;
	ET_ITERATE(children(), const Element::Pointer&, i, childrenHavingFlagCallback(flag, list, i))
	return list;
}

Element::Pointer Element::childWithNameCallback(const std::string& name, Element::Pointer root, ElementType ofType)
{
	if (root->isKindOf(ofType) && (root->name() == name)) return root;

	for (auto& i : root->children())
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

	ET_ITERATE(root->children(), const Element::Pointer&, i,  
	{
		childrenOfTypeCallback(t, list, i);
	})
}

void Element::childrenHavingFlagCallback(size_t flag, Element::List& list, Element::Pointer root)
{
	if (root->hasFlag(flag))
		list.push_back(root);

	ET_ITERATE(root->children(), const Element::Pointer&, i,  
	{
		childrenHavingFlagCallback(flag, list, i);
	})
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
		ET_ITERATE(_properites, const std::string&, i, serializeString(stream, i))
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
}

void Element::serializeChildren(std::ostream& stream, SceneVersion version)
{
	serializeInt(stream, children().size());
	ET_ITERATE(children(), Element::Pointer&, i, 
	{
		serializeInt(stream, i->type());
		i->serialize(stream, version);
	})
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
	ET_ITERATE(properties(), auto, p, object->addPropertyString(p));
	object->tag = tag;
}

void Element::serialize(std::ostream&, SceneVersion)
{
	log::error("Serialization method isn't defined for %s", typeid(*this).name());
#if (ET_DEBUG)
	abort();
#endif
}

void Element::deserialize(std::istream&, ElementFactory*, SceneVersion)
{
	log::error("Deserialization method isn't defined for %s", typeid(*this).name());
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
