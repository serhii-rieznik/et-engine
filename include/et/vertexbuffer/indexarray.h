/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/rendering.h>

namespace et
{
	class IndexArray : public Object
	{
	public:
		ET_DECLARE_POINTER(IndexArray)
		
		static const uint32_t MaxIndex;
		static const uint16_t MaxShortIndex;
		static const uint8_t MaxSmallIndex;

		int tag = 0;

	public:
		IndexArray(IndexArrayFormat format, size_t size, PrimitiveType primitiveType);
		
		void linearize(size_t size);
		void linearize(size_t indexFrom, size_t indexTo, uint32_t startIndex);

		char* binary()
			{ return _data.binary(); }
		
		const unsigned char* data() const
			{ return _data.data(); }

		size_t dataSize() const
			{ return _data.dataSize(); }

		uint32_t capacity() const
			{ return static_cast<uint32_t>(_data.dataSize()) / static_cast<uint32_t>(_format); }

		IndexArrayFormat format() const
			{ return _format; }

		PrimitiveType primitiveType() const
			{ return _primitiveType; }

		size_t actualSize() const
			{ return _actualSize; }

		void setActualSize(size_t value)
			{ _actualSize = value; }

		size_t primitivesCount() const;

		uint32_t getIndex(size_t pos) const;
		void setIndex(uint32_t value, size_t pos);
		void push_back(uint32_t value);

		void resize(size_t count);
		void resizeToFit(size_t count);

		void compact();

		class Primitive
		{
		public:
			enum
			{
				VertexCount_max = 3
			};

			size_t index[VertexCount_max];
			
		public:
			Primitive();
			
			size_t& operator [] (size_t i)
				{ ET_ASSERT(i < VertexCount_max); return index[i]; }

			const size_t& operator [] (size_t i) const
				{ ET_ASSERT(i < VertexCount_max); return index[i]; }

			bool operator == (const Primitive& p) const;
			bool operator != (const Primitive& p) const;
		};

		class PrimitiveIterator
		{
		public:
			PrimitiveIterator& operator ++();
			PrimitiveIterator operator ++(int);

			bool operator == (const PrimitiveIterator& p) const;
			bool operator != (const PrimitiveIterator& p) const;

			const Primitive& operator *() const
				{ return _primitive; }

			Primitive& operator *()
				{ return _primitive; }

			const Primitive& operator ->() const
				{ return _primitive; }
			
			Primitive& operator ->()
				{ return _primitive; }

			size_t pos() const
				{ return _pos; }
			
			size_t& operator [] (size_t i)
				{ return _primitive[i]; }
			
			const size_t& operator [] (size_t i) const
				{ return _primitive[i]; }

		private:
			friend class IndexArray;

			PrimitiveIterator(const IndexArray* ib, size_t p);
			void configure(size_t p);

			PrimitiveIterator& operator = (const PrimitiveIterator& p);

		private:
			const IndexArray* _ib = nullptr;
			Primitive _primitive;
			size_t _pos = 0;
		};

		PrimitiveIterator begin() const;
		PrimitiveIterator end() const;
		PrimitiveIterator primitive(size_t index) const;
		
	private:
		BinaryDataStorage _data;
		size_t _actualSize = 0;
		IndexArrayFormat _format = IndexArrayFormat::Format_16bit;
		PrimitiveType _primitiveType = PrimitiveType::Points;
	};

	typedef std::vector<IndexArray::Pointer> IndexArrayList;
}
