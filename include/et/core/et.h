/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iosfwd>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <string>
#include <thread>
#include <vector>

#define ET_MAJOR_VERSION			0
#define ET_MINOR_VERSION			9

#define ET_CORE_INCLUDES

#define ET_DECLARE_PIMPL(T, SZ)		private:\
										T##Private* _private = nullptr; \
										char _privateData[SZ] { }

#define ET_PIMPL_INIT(T, ...)		static_assert(sizeof(_privateData) >= sizeof(T##Private), "Not enough storage for private implementation"); \
									memset(_privateData, 0, sizeof(_privateData)); \
									_private = new (_privateData) T##Private(__VA_ARGS__);

#define ET_PIMPL_FINALIZE(T)		ET_ASSERT(_private != nullptr); \
									_private->~T##Private(); \
									_private = nullptr; \
									memset(_privateData, 0, sizeof(_privateData));

#include <et/platform/compileoptions.h>
#include <et/platform/platform.h>
#include <et/core/debug.h>
#include <et/core/constants.h>
#include <et/core/memory.h>
#include <et/core/memoryallocator.h>
#include <et/core/heapcontroller.h>
#include <et/core/intrusiveptr.h>
#include <et/core/log.h>

namespace et
{
ObjectFactory& sharedObjectFactory();
BlockMemoryAllocator& sharedBlockAllocator();
std::vector<log::Output::Pointer>& sharedLogOutputs();

template <typename T>
struct SharedBlockAllocatorSTDProxy
{
	using size_type = size_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;
	using const_reference = const T&;

	SharedBlockAllocatorSTDProxy()
	{
	}

	SharedBlockAllocatorSTDProxy(const SharedBlockAllocatorSTDProxy&)
	{
	}

	template<class U>
	SharedBlockAllocatorSTDProxy(const SharedBlockAllocatorSTDProxy<U>&)
	{
	}

	pointer allocate(size_type n)
	{
		return reinterpret_cast<pointer>(sharedBlockAllocator().allocate(n * sizeof(T)));
	}

	void deallocate(pointer ptr, size_type)
	{
		sharedBlockAllocator().release(ptr);
	}

	template<class U, class... Args>
	void construct(U* p, Args&&... args)
	{
		new ((void*)p) U(std::forward<Args>(args)...);
	}

	template<class U>
	void destroy(U* p)
	{
		p->~U();
	}

	bool operator == (const SharedBlockAllocatorSTDProxy<T>&) const
	{
		return true;
	}

	bool operator != (const SharedBlockAllocatorSTDProxy<T>&) const
	{
		return false;
	}

	template<class O>
	struct rebind
	{
		typedef SharedBlockAllocatorSTDProxy<O> other;
	};
};

template <class C, typename ... args>
C* etCreateObject(args&&... a)
{
	return sharedObjectFactory().createObject<C>(std::forward<args>(a)...);
}

template <class C>
void etDestroyObject(C* c)
{
	sharedObjectFactory().deleteObject(c);
}

using String = std::basic_string<char, std::char_traits<char>,
	SharedBlockAllocatorSTDProxy<char>>;

using WideString = std::basic_string<wchar_t, std::char_traits<wchar_t>,
	SharedBlockAllocatorSTDProxy<wchar_t>>;

template <typename T>
using Vector = std::vector<T,
	SharedBlockAllocatorSTDProxy<T>>;

template <typename T>
using Set = std::set<T, std::less<T>,
	SharedBlockAllocatorSTDProxy<T>>;

template <typename Key, typename Value>
using Map = std::map<Key, Value, std::less<Key>,
	SharedBlockAllocatorSTDProxy<std::pair<const Key, Value>>>;

template <typename Key, typename Value>
using UnorderedMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
	SharedBlockAllocatorSTDProxy<std::pair<const Key, Value>>>;

template <typename T>
struct DefaultDeleter
{
	void operator () (T* ptr)
	{
		etDestroyObject<T>(ptr);
	}
};

template <typename T>
using UniquePtr = std::unique_ptr<T, DefaultDeleter<T>>;

template <typename T, typename ... Arg>
UniquePtr<T> makeUnique(Arg&&... arg)
{
	return UniquePtr<T>(etCreateObject<T>(std::forward<Arg>(arg...)...));
}

template <uint32_t val, uint32_t al>
class AlignUpTo
{
	static_assert(al > 0, "Invalid alignment");
	enum : uint32_t { m = al - 1, e = m & (~m) };
public:
	enum : uint32_t
	{
		value = val + e
	};
};

inline uint32_t alignUpTo(uint32_t sz, uint32_t al)
{
	ET_ASSERT(al > 0);
	uint32_t m = al - 1;
	return sz + m & (~m);
}

inline uint32_t alignDownTo(uint32_t sz, uint32_t al)
{
	ET_ASSERT(al > 0);
	return sz & (~(al - 1));
}
}

#include <et/core/strings.hpp>
#include <et/core/threading.h>
#include <et/core/filesystem.h>
#include <et/core/conversionbase.h>
#include <et/core/object.h>
#include <et/core/stream.h>
#include <et/core/hierarchy.h>
#include <et/core/dictionary.h>

#include <et/geometry/vector4.h>
#include <et/geometry/matrix3.h>
#include <et/geometry/matrix4.h>
#include <et/geometry/quaternion.h>
#include <et/geometry/rect.h>

#undef ET_CORE_INCLUDES

namespace et
{
typedef vector2<float> vec2;
typedef vector3<float> vec3;
typedef vector4<float> vec4;

typedef vector2<int32_t> vec2i;
typedef vector3<int32_t> vec3i;
typedef vector4<int32_t> vec4i;

typedef vector2<uint8_t> vec2ub;
typedef vector3<uint8_t> vec3ub;
typedef vector4<uint8_t> vec4ub;

typedef matrix3<float> mat3;
typedef matrix4<float> mat4;

typedef matrix3<int32_t> mat3i;
typedef matrix4<int32_t> mat4i;

typedef Quaternion<float> quaternion;

using recti = Rect<int32_t>;
using rectf = Rect<float>;

template<typename T>
inline T clamp(T value, T min, T max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

extern const vec3 unitX;
extern const vec3 unitY;
extern const vec3 unitZ;
extern const mat3 identityMatrix3;
extern const mat4 identityMatrix;
extern const mat4 lightProjectionMatrix;

extern const std::string emptyString;
}
