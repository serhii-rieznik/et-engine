/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>

namespace et
{

class Light : public Camera
{
public:
	ET_DECLARE_POINTER(Light);

	enum class Type : uint32_t
	{
		UniformColorSky,
		ImageBasedSky,
		Directional,
	};
public:
	Light(Type);

	const Type type() const 
		{ return _type; }

	void setEnvironmentMap(const std::string&);
	
	const std::string& environmentMap() const
		{ return _environmentMap; }

private:
	Type _type = Type::Directional;
	std::string _environmentMap;
};

}
