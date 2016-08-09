/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

namespace et
{

const std::string emptyString;

static struct SharedEngineObjects
{
	ObjectFactory objectFactory;
	BlockMemoryAllocator blockMemoryAllocator;
	std::vector<log::Output::Pointer> logOutputs;
	StringList emptyStringList;
	
	SharedEngineObjects();
}
sharedEngineObjects;

const StringList& emptyStringList()
{
	return sharedEngineObjects.emptyStringList;
}

SharedEngineObjects::SharedEngineObjects()
{
	objectFactory.setAllocator(&blockMemoryAllocator);
}

BlockMemoryAllocator& sharedBlockAllocator()
{
	return sharedEngineObjects.blockMemoryAllocator;
}

ObjectFactory& sharedObjectFactory()
{
	return sharedEngineObjects.objectFactory;
}

std::vector<log::Output::Pointer>& sharedLogOutputs()
{
	return sharedEngineObjects.logOutputs;
}

}