/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/objects/light.h>

namespace et
{

Light::Light(Type type) : 
	_type(type)
{
}

void Light::setEnvironmentMap(const std::string& envMap)
{
	_environmentMap = envMap;
}

}
