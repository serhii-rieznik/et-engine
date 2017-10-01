/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
class ObjectsCache;

struct Object : public Shared
{
	ET_DECLARE_POINTER(Object);

	Object() = default;
	virtual ~Object() = default;
};

class NamedObject : public Object
{
public:
	NamedObject() = default;

	NamedObject(const std::string& aName) :
		_name(aName)
	{
	}

	const std::string& name() const
	{
		return _name;
	}

	void setName(const std::string& name)
	{
		_name = name;
	}

private:
	std::string _name;
};

class LoadableObject : public NamedObject
{
public:
	ET_DECLARE_POINTER(LoadableObject);
	using Collection = Vector<Pointer>;

public:
	LoadableObject() = default;

	LoadableObject(const std::string& aName) :
		NamedObject(aName)
	{
	}

	LoadableObject(const std::string& aName, const std::string& aOrigin) :
		NamedObject(aName), _origin(aOrigin)
	{
	}

	const std::string& origin() const
	{
		return _origin;
	}

	void setOrigin(const std::string& origin)
	{
		_origin = origin;
	}

	const StringList& distributedOrigins() const
	{
		return _distributedOrigins;
	}

	void addOrigin(const std::string& origin)
	{
		ET_ASSERT(!origin.empty()); _distributedOrigins.push_back(origin);
	}

	bool canBeReloaded() const
	{
		return !(_origin.empty() && _distributedOrigins.empty());
	}

private:
	std::string _origin;
	StringList _distributedOrigins;
};

struct ObjectLoader : public Object
{
	ET_DECLARE_POINTER(ObjectLoader);

	virtual void reloadObject(LoadableObject::Pointer, ObjectsCache&) = 0;
};

}
