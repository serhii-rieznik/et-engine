/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <ostream>

#include <et/core/containers.h>

namespace et
{
	inline void serializeInt32(std::ostream& stream, int32_t value)
	{
		ET_ASSERT(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), 4);
	}
	inline int32_t deserializeInt32(std::istream& stream)
	{
		ET_ASSERT(stream.good());
		int32_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), 4);
		return value;
	}
	inline void serializeInt64(std::ostream& stream, int64_t value)
	{
		ET_ASSERT(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), 8);
	}
	inline int64_t deserializeInt64(std::istream& stream)
	{
		ET_ASSERT(stream.good());
		
		int64_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), 8);
		return value;
	}

	inline void serializeUInt32(std::ostream& stream, uint32_t value)
	{
		ET_ASSERT(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), 4);
	}
	inline uint32_t deserializeUInt32(std::istream& stream)
	{
		ET_ASSERT(stream.good());
		
		uint32_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), 4);
		return value;
	}
	inline void serializeUInt64(std::ostream& stream, uint64_t value)
	{
		ET_ASSERT(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), 8);
	}
	inline uint64_t deserializeUInt64(std::istream& stream)
	{
		ET_ASSERT(stream.good());
		
		uint64_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), 8);
		return value;
	}
	
	inline void serializeString(std::ostream& stream, const std::string& s)
	{
		ET_ASSERT(stream.good());

		serializeUInt64(stream, s.size());
        
		if (s.size() > 0)
			stream.write(s.c_str(), static_cast<std::streamsize>(s.size()));
	}

	inline std::string deserializeString(std::istream& stream)
	{
		ET_ASSERT(stream.good());

		uint64_t size = deserializeUInt64(stream);
		if (size > 0)
		{
			StringDataStorage value(size + 1, 0);
			stream.read(value.binary(), size);
			return std::string(value.binary());
		}

		return emptyString;
	}

	inline void serializeFloat(std::ostream& stream, float value)
	{
		ET_ASSERT(stream.good());

		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	inline float deserializeFloat(std::istream& stream)
	{
		ET_ASSERT(stream.good());

		float value = 0;
		stream.read(reinterpret_cast<char*>(&value), sizeof(value)); 
		return value;
	}

	template <typename T>
	inline void serializeVector(std::ostream& stream, const T& value)
	{
		ET_ASSERT(stream.good());

		stream.write(value.binary(), sizeof(value)); 
	}

	template <typename T>
	inline T deserializeVector(std::istream& stream)
	{
		ET_ASSERT(stream.good());

		T value;
		stream.read(value.binary(), sizeof(value)); 
		return value;
	}

	inline void serializeQuaternion(std::ostream& stream, const quaternion& value)
	{
		ET_ASSERT(stream.good());

		serializeFloat(stream, value.scalar);
		serializeVector(stream, value.vector);
	}

	inline quaternion deserializeQuaternion(std::istream& stream)
	{
		ET_ASSERT(stream.good());

		quaternion result;
		result.scalar = deserializeFloat(stream);
		result.vector = deserializeVector<vec3>(stream);
		return result;
	}

	inline void serializeMatrix(std::ostream& stream, const mat4& value)
	{
		ET_ASSERT(stream.good());

		stream.write(value.binary(), sizeof(value));
	}

	inline mat4 deserializeMatrix(std::istream& stream)
	{
		ET_ASSERT(stream.good());

		mat4 value;
		stream.read(value.binary(), sizeof(value));
		return value;
	}

}
