/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <ostream>

#include <et/core/containers.h>
#include <et/geometry/geometry.h>

namespace et
{
	inline void serializeInt(std::ostream& stream, int32_t value)
	{
		assert(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	inline void serializeInt(std::ostream& stream, uint32_t value)
	{
		assert(stream.good());
		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

#if (!ET_PLATFORM_WIN32)
	inline void serializeInt(std::ostream& stream, size_t value)
		{ serializeInt(stream, static_cast<uint32_t>(value)); }
#endif
    
	inline int32_t deserializeInt(std::istream& stream)
	{
		assert(stream.good());

		int value = 0;
		stream.read(reinterpret_cast<char*>(&value), sizeof(value)); 
		return value;
	}
	
	inline uint32_t deserializeUInt(std::istream& stream)
	{
		assert(stream.good());
		
		uint32_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	
	inline size_t deserializeSizeT(std::istream& stream)
	{
		assert(stream.good());
		
		size_t value = 0;
		stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
		return value;
	}

	inline void serializeString(std::ostream& stream, const std::string& s)
	{
		assert(stream.good());

		serializeInt(stream, s.size());
        
		if (s.size() > 0)
			stream.write(s.c_str(), static_cast<std::streamsize>(s.size()));
	}

	inline std::string deserializeString(std::istream& stream)
	{
		assert(stream.good());

		int size = deserializeInt(stream);
		if (size > 0)
		{
			StringDataStorage value(size + 1, 0);
			stream.read(value.binary(), size);
			return std::string(value.binary());
		}

		return std::string();
	}

	inline void serializeFloat(std::ostream& stream, float value)
	{
		assert(stream.good());

		stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	inline float deserializeFloat(std::istream& stream)
	{
		assert(stream.good());

		float value = 0;
		stream.read(reinterpret_cast<char*>(&value), sizeof(value)); 
		return value;
	}

	template <typename T>
	inline void serializeVector(std::ostream& stream, const T& value)
	{
		assert(stream.good());

		stream.write(value.binary(), sizeof(value)); 
	}

	template <typename T>
	inline T deserializeVector(std::istream& stream)
	{
		assert(stream.good());

		T value;
		stream.read(value.binary(), sizeof(value)); 
		return value;
	}

	inline void serializeQuaternion(std::ostream& stream, const quaternion& value)
	{
		assert(stream.good());

		serializeFloat(stream, value.scalar);
		serializeVector(stream, value.vector);
	}

	inline quaternion deserializeQuaternion(std::istream& stream)
	{
		assert(stream.good());

		quaternion result;
		result.scalar = deserializeFloat(stream);
		result.vector = deserializeVector<vec3>(stream);
		return result;
	}

	inline void serializeMatrix(std::ostream& stream, const mat4& value)
	{
		assert(stream.good());

		stream.write(value.binary(), sizeof(value));
	}

	inline mat4 deserializeMatrix(std::istream& stream)
	{
		assert(stream.good());

		mat4 value;
		stream.read(value.binary(), sizeof(value));
		return value;
	}

}