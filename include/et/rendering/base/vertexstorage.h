/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/rawdataaccessor.h>
#include <et/rendering/base/vertexarray.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{
	template <DataType T>
	class VertexDataAccessor { };
	
	class VertexStoragePrivate;
	class VertexStorage : public Object
	{
	public:
		ET_DECLARE_POINTER(VertexStorage);

	public:
		VertexStorage(const VertexDeclaration&, uint32_t);
		VertexStorage(const VertexArray::Pointer&);
		~VertexStorage();
		
		template <DataType T>
		VertexDataAccessor<T> accessData(VertexAttributeUsage usage, uint32_t offset)
		{
			ET_ASSERT(hasAttributeWithType(usage, T));
			
			return VertexDataAccessor<T>(data().binary(), static_cast<uint32_t>(data().dataSize()), 
				stride(), offset * stride() + offsetOfAttribute(usage));
		}
		
		template <DataType T>
		VertexDataAccessor<T> accessData(VertexAttributeUsage usage, uint32_t offset) const
		{
			ET_ASSERT(hasAttributeWithType(usage, T));
			
			return VertexDataAccessor<T>(data().binary(), static_cast<uint32_t>(data().dataSize()),
                stride(), offset * stride() + offsetOfAttribute(usage));
		}
		
		BinaryDataStorage& data();
		const BinaryDataStorage& data() const;
		
		void increaseSize(uint32_t);
		void resize(uint32_t);
		
		bool hasAttribute(VertexAttributeUsage) const;
		bool hasAttributeWithType(VertexAttributeUsage, DataType) const;
		DataType attributeType(VertexAttributeUsage) const;
		
		const VertexDeclaration& declaration() const;
		
		uint32_t stride() const;
		uint32_t capacity() const;
		
		uint32_t offsetOfAttribute(VertexAttributeUsage usage) const;
		uint32_t sizeOfAttribute(VertexAttributeUsage usage) const;
		
	private:
		ET_DECLARE_PIMPL(VertexStorage, 128);
	};
	
#define ET_DECLARE_ACCESSOR(vat, base) \
	template <> class VertexDataAccessor<DataType::vat> : public RawDataAcessor<base> \
	{ \
	public: \
		VertexDataAccessor() { } \
		VertexDataAccessor(const char* data, uint32_t dataSize, uint32_t stride, uint32_t offset) : \
			RawDataAcessor<base>(data, dataSize, stride, offset) { } \
		VertexDataAccessor(char* data, uint32_t dataSize, uint32_t stride, uint32_t offset) : \
			RawDataAcessor<base>(data, dataSize, stride, offset) { } \
	};
	
	ET_DECLARE_ACCESSOR(Int, int32_t)
	ET_DECLARE_ACCESSOR(Float, float)
	ET_DECLARE_ACCESSOR(Vec2, vec2)
	ET_DECLARE_ACCESSOR(IntVec2, vec2i)
	ET_DECLARE_ACCESSOR(Vec3, vec3)
	ET_DECLARE_ACCESSOR(IntVec3, vec3i)
	ET_DECLARE_ACCESSOR(Vec4, vec4)
	ET_DECLARE_ACCESSOR(IntVec4, vec4i)
#undef ET_DECLARE_ACCESSOR
}
