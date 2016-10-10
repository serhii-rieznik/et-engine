/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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
		
		VertexElement(VertexAttributeUsage aUsage, DataType aType, uint32_t aStride = 0,
			uint32_t aOffset = 0);

		bool operator == (const VertexElement& r) const
			{ return (_usage == r._usage) && (_type == r._type) && (_stride == r._stride) && (_offset == r._offset); }

		bool operator != (const VertexElement& r) const
			{ return (_usage != r._usage) || (_type != r._type) || (_stride != r._stride) || (_offset != r._offset); }

		bool operator == (const VertexAttributeUsage& aUsage) const
			{ return (_usage == aUsage); }

		VertexAttributeUsage usage() const
			{ return _usage; }

		DataType type() const
			{ return _type; } 

		uint32_t stride() const
			{ return _stride; }

		uint32_t offset() const
			{ return _offset; }

		uint32_t components() const
			{ return _components; }

		DataFormat dataFormat() const
			{ return _dataFormat; }

		void setStride(int s)
			{ _stride = s; }

		bool operator < (const VertexElement& r) const
			{ return _usage < r._usage; }

	private:
		VertexAttributeUsage _usage = VertexAttributeUsage::Position;
		DataType _type = DataType::Float;
		DataFormat _dataFormat = DataFormat::Float;
		uint32_t _stride = 0;
		uint32_t _offset = 0;
		uint32_t _components = 0;
	};
    using VertexElementSet = Set<VertexElement>;

	class VertexDeclaration
	{
	public:
		VertexDeclaration();
		VertexDeclaration(bool interleaved);
		VertexDeclaration(bool interleaved, VertexAttributeUsage usage, DataType type);

		bool has(VertexAttributeUsage usage) const;

		bool push_back(VertexAttributeUsage usage, DataType type);
		bool push_back(const VertexElement& element);

		bool remove(VertexAttributeUsage usage);
		void clear();

		const VertexElement& element(uint32_t) const;
		const VertexElement& elementForUsage(VertexAttributeUsage) const;

		VertexElementSet& elements()
			{ return _elements; }
		
		const VertexElementSet& elements() const
			{ return _elements; }   

		const VertexElement& operator [](uint32_t i) const
			{ return element(i); }

		uint32_t numElements() const
			{ return static_cast<uint32_t>(_elements.size()); }

		uint32_t totalSize() const
			{ return _totalSize; }

		bool interleaved() const
			{ return _interleaved; }

		bool operator == (const VertexDeclaration& r) const;
		
		bool operator != (const VertexDeclaration& r) const
			{ return !(operator == (r)); }

		bool hasSameElementsAs(const VertexDeclaration&) const;

	private:  
		VertexElementSet _elements;
		uint32_t _totalSize = 0;
		uint32_t _usageMask = 0;
		bool _interleaved = true;
	};

}
