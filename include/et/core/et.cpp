/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

namespace et
{

const std::string emptyString;

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
