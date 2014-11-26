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
		enum SerializationFlags
		{
			SerializationFlag_ReadableFormat = 0x01,
			SerializationFlag_ConvertUnicode = 0x02
		};
		
		std::string serialize(const et::Dictionary&, size_t = 0);
		std::string serialize(const et::ArrayValue&, size_t = 0);
		
		et::ValueBase::Pointer deserialize(const char*, et::ValueClass&, bool printErrors = true);
		et::ValueBase::Pointer deserialize(const std::string&, et::ValueClass&, bool printErrors = true);
	}
}
