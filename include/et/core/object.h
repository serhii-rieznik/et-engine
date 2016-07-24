/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	class Object : public Shared
	{
	public:
		ET_DECLARE_POINTER(Object);
		
	public:
		Object() = default;

		Object(const std::string& aName) :
			_name(aName) { }

		virtual ~Object() = default;

		const std::string& name() const
			{ return _name; }

		void setName(const std::string& name)
			{ _name = name; }

	private:
		std::string _name;
	};

	class RenderContext;
	class ObjectsCache;

	class LoadableObject : public Object
	{
	public:
		ET_DECLARE_POINTER(LoadableObject);
		
	public:
		LoadableObject() = default;

		LoadableObject(const std::string& aName) :
			Object(aName) { }
		
		LoadableObject(const std::string& aName, const std::string& aOrigin) :
			Object(aName), _origin(aOrigin) { }

		const std::string& origin() const
			{ return _origin; }
		
		void setOrigin(const std::string& origin)
			{ _origin = origin; }
		
		const StringList& distributedOrigins() const
			{ return _distributedOrigins; }
		
		void addOrigin(const std::string& origin)
			{ _distributedOrigins.push_back(origin); }

		bool canBeReloaded() const
			{ return !(_origin.empty() && _distributedOrigins.empty()); }
	
	private:
		std::string _origin;
		StringList _distributedOrigins;
	};
	
	class ObjectLoader : public Shared
	{
	public:
		ET_DECLARE_POINTER(ObjectLoader);
		
	public:
		virtual ~ObjectLoader() = default;
		virtual void reloadObject(LoadableObject::Pointer, ObjectsCache&) = 0;
	};
}
