/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
