/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/properties.h>
#include <et/camera/camera.h>

namespace et
{
	class Light : public Camera
	{
	public:
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(vec4, ambientColor, setAmbientColor)
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(vec4, diffuseColor, setDiffuseColor)
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(vec4, specularColor, setSpecularColor)
	};
}