/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/criticalsection.h>
#include <et/core/timedobject.h>

namespace et
{
	class ObjectsCache : public TimedObject
	{
	public:
		ObjectsCache();
		~ObjectsCache();

		void manage(const LoadableObject::Pointer&, const ObjectLoader::Pointer& loader);
		void discard(const LoadableObject::Pointer& o);
		
		void clear();
		void flush();

		std::vector<LoadableObject::Pointer> findObjects(const std::string& key);
		LoadableObject::Pointer findAnyObject(const std::string& key, uint64_t* property = nullptr);

		void startMonitoring();
		void stopMonitoring();
		void report();
		
		uint64_t getFileProperty(const std::string& p);
		uint64_t getObjectProperty(LoadableObject::Pointer);

	private:
		ET_DENY_COPY(ObjectsCache);

		void performUpdate();
		void update(float t);

	private:
		struct ObjectProperty
		{
			LoadableObject::Pointer object;
			ObjectLoader::Pointer loader;
			std::unordered_map<std::string, uint64_t> identifiers;

			ObjectProperty()
				{ }
			
			ObjectProperty(LoadableObject::Pointer o, ObjectLoader::Pointer l) :
				object(o), loader(l) { }
		};
		
		using ObjectPropertyList = Vector<ObjectProperty>;
		using ObjectMap = UnorderedMap<std::string, ObjectPropertyList>;

		CriticalSection _lock;
		ObjectMap _objects;
		float _updateTime = 0.0f;
	};
}
