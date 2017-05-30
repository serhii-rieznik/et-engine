/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/objects/light.h>

namespace et
{
namespace s3d
{

class LightElement : public BaseElement
{
public:
	ET_DECLARE_POINTER(LightElement);

public:
	LightElement(BaseElement* parent = nullptr);
	LightElement(const Light::Pointer&, BaseElement* parent = nullptr);

	ElementType type() const override { return ElementType::Light; };

	Light::Pointer& light() { return _light; }
	const Light::Pointer& light() const { return _light; }

	BaseElement* duplicate() override;

private:
	Light::Pointer _light;
};

}
}
