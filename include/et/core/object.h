/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	class Object : public Shared
	{
	public:
		ET_DECLARE_POINTER(Object)
		
	public:
		Object()
			{ }

		Object(const std::string& aName) :
			_name(aName) { }

		virtual ~Object()
			{ }

	public:
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(std::string, name, setName)
	};

	class RenderContext;
	class ObjectsCache;

	class LoadableObject : public Object
	{
	public:
		ET_DECLARE_POINTER(LoadableObject)
		
	public:
		LoadableObject()
			{ }

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
		ET_DECLARE_POINTER(ObjectLoader)
		
	public:
		virtual ~ObjectLoader() { }
		virtual void reloadObject(LoadableObject::Pointer, ObjectsCache&) = 0;
	};
}
