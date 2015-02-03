/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/rawdataaccessor.h>
#include <et/vertexbuffer/vertexarray.h>
#include <et/vertexbuffer/vertexdeclaration.h>

namespace et
{
	template <VertexAttributeType T>
	class VertexDataAccessor { };
	
	class VertexStoragePrivate;
	class VertexStorage : public Shared
	{
	public:
		ET_DECLARE_POINTER(VertexStorage)
		
		int tag = 0;
		
	public:
		VertexStorage(const VertexDeclaration&, size_t);
		VertexStorage(const VertexArray::Pointer&);
		~VertexStorage();
		
		template <VertexAttributeType T>
		VertexDataAccessor<T> accessData(VertexAttributeUsage usage, size_t)
		{
			ET_ASSERT(hasAttributeWithType(usage, T));
			return VertexDataAccessor<T>(data().binary(), data().dataSize(), stride(), offsetOfAttribute(usage));
		}
		
		template <VertexAttributeType T>
		VertexDataAccessor<T> accessData(VertexAttributeUsage usage, size_t) const
		{
			ET_ASSERT(hasAttributeWithType(usage, T));
			return VertexDataAccessor<T>(data().binary(), data().dataSize(), stride(), offsetOfAttribute(usage));
		}
		
		BinaryDataStorage& data();
		const BinaryDataStorage& data() const;
		
		bool hasAttribute(VertexAttributeUsage) const;
		bool hasAttributeWithType(VertexAttributeUsage, VertexAttributeType) const;
		
		const VertexDeclaration& declaration() const;
		
		size_t stride() const;
		size_t offsetOfAttribute(VertexAttributeUsage usage) const;
		size_t sizeOfAttribute(VertexAttributeUsage usage) const;
		
	private:
		ET_DECLARE_PIMPL(VertexStorage, 128)
	};
	
	template <> class VertexDataAccessor<VertexAttributeType::Int> : public RawDataAcessor<uint32_t>
	{
	public:
		VertexDataAccessor() { }
		
		VertexDataAccessor(const char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<uint32_t>(data, dataSize, stride, offset) { }
		
		VertexDataAccessor(char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<uint32_t>(data, dataSize, stride, offset) { }
	};
	template <> class VertexDataAccessor<VertexAttributeType::Vec2> : public RawDataAcessor<vec2>
	{
	public:
		VertexDataAccessor() { }
		
		VertexDataAccessor(const char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec2>(data, dataSize, stride, offset) { }
		
		VertexDataAccessor(char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec2>(data, dataSize, stride, offset) { }
	};
	template <> class VertexDataAccessor<VertexAttributeType::Vec3> : public RawDataAcessor<vec3>
	{
	public:
		VertexDataAccessor() { }
		
		VertexDataAccessor(const char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec3>(data, dataSize, stride, offset) { }

		VertexDataAccessor(char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec3>(data, dataSize, stride, offset) { }
	};
	template <> class VertexDataAccessor<VertexAttributeType::Vec4> : public RawDataAcessor<vec4>
	{
	public:
		VertexDataAccessor() { }
		
		VertexDataAccessor(const char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec4>(data, dataSize, stride, offset) { }

		VertexDataAccessor(char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<vec4>(data, dataSize, stride, offset) { }
	};
	template <> class VertexDataAccessor<VertexAttributeType::Float> : public RawDataAcessor<float>
	{
	public:
		VertexDataAccessor() { }
		
		VertexDataAccessor(const char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<float>(data, dataSize, stride, offset) { }

		VertexDataAccessor(char* data, size_t dataSize, size_t stride, size_t offset) :
			RawDataAcessor<float>(data, dataSize, stride, offset) { }
	};
}
