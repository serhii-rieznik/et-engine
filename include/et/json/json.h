/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	namespace json
	{
		std::string serialize(const et::Dictionary&, bool readableFormat = false);
		std::string serialize(const et::ArrayValue&, bool readableFormat = false);
		
		et::ValueBase::Pointer deserialize(const char*, et::ValueClass&, bool printErrors = true);
		et::ValueBase::Pointer deserialize(const std::string&, et::ValueClass&, bool printErrors = true);
	}
}
