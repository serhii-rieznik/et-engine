/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

namespace et
{
	class VertexElement
	{
	public:
		VertexElement() : 
			_usage(Usage_Undefined), _type(Type_Undefined), _stride(0), _offset(0), _components(0), _dataType(0) { }

		VertexElement(VertexAttributeUsage aUsage, VertexAttributeType aType, int aStride = 0, size_t aOffset = 0) : 
			_usage(aUsage), _type(aType), _stride(aStride), _offset(aOffset),
			_components(vertexAttributeTypeComponents(aType)), _dataType(vertexAttributeTypeDataType(aType)) { }

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

		int stride() const
			{ return _stride; }

		size_t offset() const
			{ return _offset; }

		size_t components() const
			{ return _components; }

		uint32_t dataType() const
			{ return _dataType; }

		void setStride(int s)
			{ _stride = s; }

	private:
		VertexAttributeUsage _usage;
		VertexAttributeType _type; 
		int _stride;
		size_t _offset;
		size_t _components;
		uint32_t _dataType;
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

		size_t dataSize() const
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
		size_t _totalSize = 0;
		size_t _usageMask = 0;
		bool _interleaved = true;
	};

}
