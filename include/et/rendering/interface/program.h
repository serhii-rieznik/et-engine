/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

namespace et
{
class Program : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Program);

public:
	virtual void build(const std::string& source) = 0;
};
}
