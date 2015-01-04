/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

using namespace et;

const std::string et::emptyString;

static struct SharedEngineObjects
{
	ObjectFactory objectFactory;
	BlockMemoryAllocator blockMemoryAllocator;
	std::vector<log::Output::Pointer> logOutputs;
	
	SharedEngineObjects();
}
sharedEngineObjects;

SharedEngineObjects::SharedEngineObjects()
{
	objectFactory.setAllocator(&blockMemoryAllocator);
}

BlockMemoryAllocator& et::sharedBlockAllocator()
{
	return sharedEngineObjects.blockMemoryAllocator;
}

ObjectFactory& et::sharedObjectFactory()
{
	return sharedEngineObjects.objectFactory;
}

std::vector<log::Output::Pointer>& et::sharedLogOutputs()
{
	return sharedEngineObjects.logOutputs;
}
