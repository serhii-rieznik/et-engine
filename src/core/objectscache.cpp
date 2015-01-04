/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/objectscache.h>

using namespace et;

ObjectsCache::ObjectsCache() :
	_updateTime(0.0f)
{
}

ObjectsCache::~ObjectsCache()
{
	clear();
}

void ObjectsCache::manage(const LoadableObject::Pointer& o, const ObjectLoader::Pointer& loader)
{
	if (o.valid() && o->canBeReloaded())
	{
		CriticalSectionScope lock(_lock);
		
		if (_objects.count(o->origin()) > 0)
		{
			ObjectPropertyList& list = _objects[o->origin()];
			list.push_back(ObjectProperty(o, loader));
			ObjectProperty& newObject = list.back();
			newObject.identifiers[o->origin()] = getFileProperty(o->origin());
			for (auto& s : o->distributedOrigins())
				newObject.identifiers[s] = getFileProperty(s);
		}
		else
		{
			ObjectPropertyList newList(1, ObjectProperty(o, loader));
			ObjectProperty& newObject = newList.back();
			newObject.identifiers[o->origin()] = getFileProperty(o->origin());
			for (auto& s : o->distributedOrigins())
				newObject.identifiers[s] = getFileProperty(s);
			_objects.insert(std::make_pair(o->origin(), newList));
		}
	}
	else
	{
		log::warning("[ObjectsCache] Trying to manage invalid object");
	}
}

std::vector<LoadableObject::Pointer> ObjectsCache::findObjects(const std::string& key)
{
	CriticalSectionScope lock(_lock);
	auto i = _objects.find(key);
	if (i == _objects.end())
		return std::vector<LoadableObject::Pointer>();
		
	std::vector<LoadableObject::Pointer> result;
	
	for (auto prop : i->second)
		result.push_back(prop.object);
	
	return result;
}

LoadableObject::Pointer ObjectsCache::findAnyObject(const std::string& key, uint64_t* property)
{
	CriticalSectionScope lock(_lock);
	
	auto i = _objects.find(key);
	if (i == _objects.end())
	{
		if (property)
			*property = 0;
		
		return  LoadableObject::Pointer();
	}
	else
	{
		if (property)
			*property = i->second.front().identifiers[key];
		
		return i->second.front().object;
	}
}


void ObjectsCache::discard(const LoadableObject::Pointer& o)
{
	if (o.valid())
	{
		CriticalSectionScope lock(_lock);
		if (_objects.count(o->origin()) == 0) return;
		
		ObjectPropertyList& list = _objects[o->origin()];
		for (auto i = list.begin(), e = list.end(); i != e; ++i)
		{
			if (i->object == o)
			{
				list.erase(i);
				break;
			}
		}
		
		if (list.empty())
			_objects.erase(o->origin());
	}
}

void ObjectsCache::clear()
{
	CriticalSectionScope lock(_lock);
	_objects.clear();
}

void ObjectsCache::flush()
{
	CriticalSectionScope lock(_lock);
	
	size_t objectsErased = 0;
	
	for (auto& lv : _objects)
	{
		auto obj = lv.second.begin();
		while (obj != lv.second.end())
		{
			if (obj->object->atomicCounterValue() == 1)
			{
				obj = lv.second.erase(obj);
				++objectsErased;
			}
			else
			{
				++obj;
			}
		}
	}
	
	auto i = _objects.begin();
	while (i != _objects.end())
	{
		if (i->second.empty())
		{
			i = _objects.erase(i);
		}
		else
		{
			++i;
		}
	}
	
	if (objectsErased > 0)
		log::info("[ObjectsCache] %llu objects flushed.", static_cast<uint64_t>(objectsErased));
}

void ObjectsCache::startMonitoring()
{
	startUpdates();
}

void ObjectsCache::stopMonitoring()
{
	cancelUpdates();
}

void ObjectsCache::update(float t)
{
	static const float updateInterval = 0.5f;
	
	if (_updateTime == 0.0f)
		_updateTime = t;
	
	float dt = t - _updateTime;

	if (dt > updateInterval)
	{
		performUpdate();
		_updateTime = t;
	}
}

uint64_t ObjectsCache::getFileProperty(const std::string& p)
{
	return getFileDate(p);
}

uint64_t ObjectsCache::getObjectProperty(LoadableObject::Pointer ptr)
{
	CriticalSectionScope lock(_lock);
	
	for (auto& entry : _objects)
	{
		for (auto& p : entry.second)
		{
			if (p.object == ptr)
				return p.identifiers[ptr->origin()];
		}
	}
	
	return 0;
}

void ObjectsCache::performUpdate()
{
	ObjectsCache& cache = *this;
	
	for (auto& entry : _objects)
	{
		for (auto& p : entry.second)
		{
			if (p.loader.valid() && p.object->canBeReloaded())
			{
				bool shouldReload = false;
				
				for (auto i : p.identifiers)
				{
					uint64_t prop = getFileProperty(i.first);
					if (prop != i.second)
					{
						i.second = prop;
						shouldReload = true;
						break;
					}
				}
				
				if (shouldReload)
					p.loader->reloadObject(p.object, cache);
			}
		}
	}
}

void ObjectsCache::report()
{
	log::info("[ObjectsCache] Contains %llu objects", static_cast<uint64_t>(_objects.size()));
}
