/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

namespace et
{
	class VertexElement
	{
	public:
		VertexElement()
			{ }
		
		VertexElement(VertexAttributeUsage aUsage, VertexAttributeType aType, uint32_t aStride = 0,
			uint32_t aOffset = 0);

		bool operator == (const VertexElement& r) const
			{ return (_usage == r._usage) && (_type == r._type) && (_stride == r._stride) && (_offset == r._offset); }

		bool operator != (const VertexElement& r) const
			{ return (_usage != r._usage) || (_type != r._type) || (_stride != r._stride) || (_offset != r._offset); }

		bool operator == (const VertexAttributeUsage& aUsage) const
			{ return (_usage == aUsage); }

		VertexAttributeUsage usage() const
			{ return _usage; }

		VertexAttributeType type() const
			{ return _type; } 

		uint32_t stride() const
			{ return _stride; }

		uint32_t offset() const
			{ return _offset; }

		uint32_t components() const
			{ return _components; }

		DataType dataType() const
			{ return _dataType; }

		void setStride(int s)
			{ _stride = s; }

	private:
		VertexAttributeUsage _usage = VertexAttributeUsage::Position;
		VertexAttributeType _type = VertexAttributeType::Float;
		DataType _dataType = DataType::Float;
		uint32_t _stride = 0;
		uint32_t _offset = 0;
		uint32_t _components = 0;
	};

	typedef std::vector<VertexElement> VertexElementList;

	class VertexDeclaration
	{
	public:
		VertexDeclaration();
		VertexDeclaration(bool interleaved);
		VertexDeclaration(bool interleaved, VertexAttributeUsage usage, VertexAttributeType type);

		bool has(VertexAttributeUsage usage) const;

		bool push_back(VertexAttributeUsage usage, VertexAttributeType type);
		bool push_back(const VertexElement& element);

		bool remove(VertexAttributeUsage usage);
		void clear();

		const VertexElement& element(size_t) const;
		const VertexElement& elementForUsage(VertexAttributeUsage) const;

		const VertexElementList& elements() const
			{ return _list; }   

		VertexElement& operator [](size_t i)
			{ return _list.at(i); }

		size_t numElements() const
			{ return _list.size(); }

		uint32_t dataSize() const
			{ return _totalSize; }

		bool interleaved() const
			{ return _interleaved; }

		bool operator == (const VertexDeclaration& r) const;
		
		bool operator != (const VertexDeclaration& r) const
			{ return !(operator == (r)); }

		void serialize(std::ostream& stream);
		void deserialize(std::istream& stream);

	private:  
		VertexElementList _list;
		uint32_t _totalSize = 0;
		uint32_t _usageMask = 0;
		bool _interleaved = true;
	};

}
