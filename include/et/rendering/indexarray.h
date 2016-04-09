/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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
		IndexArray(IndexArrayFormat format, uint32_t size, PrimitiveType primitiveType);
		
		void linearize(uint32_t size);
		void linearize(uint32_t indexFrom, uint32_t indexTo, uint32_t startIndex);

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

		uint32_t actualSize() const
			{ return _actualSize; }

		void setActualSize(uint32_t value)
			{ _actualSize = value; }

		uint32_t primitivesCount() const;

		uint32_t getIndex(uint32_t pos) const;
		void setIndex(uint32_t value, uint32_t pos);
		void push_back(uint32_t value);

		void resize(uint32_t count);
		void resizeToFit(uint32_t count);

		void compact();

		class Primitive
		{
		public:
			enum
			{
				VertexCount_max = 3
			};

			uint32_t index[VertexCount_max];
			
		public:
			Primitive();
			
			uint32_t& operator [] (uint32_t i)
				{ ET_ASSERT(i < VertexCount_max); return index[i]; }

			const uint32_t& operator [] (uint32_t i) const
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

			uint32_t pos() const
				{ return _pos; }
			
			uint32_t& operator [] (uint32_t i)
				{ return _primitive[i]; }
			
			const uint32_t& operator [] (uint32_t i) const
				{ return _primitive[i]; }

		private:
			friend class IndexArray;

			PrimitiveIterator(const IndexArray* ib, uint32_t p);
			void configure(uint32_t p);

			PrimitiveIterator& operator = (const PrimitiveIterator& p);

		private:
			const IndexArray* _ib = nullptr;
			Primitive _primitive;
			uint32_t _pos = 0;
		};

		PrimitiveIterator begin() const;
		PrimitiveIterator end() const;
		PrimitiveIterator primitive(uint32_t index) const;
		
	private:
		BinaryDataStorage _data;
		uint32_t _actualSize = 0;
		IndexArrayFormat _format = IndexArrayFormat::Format_16bit;
		PrimitiveType _primitiveType = PrimitiveType::Points;
	};
}
