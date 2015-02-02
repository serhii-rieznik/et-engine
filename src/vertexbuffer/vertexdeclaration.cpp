/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexdeclaration.h>

namespace et
{
	VertexAttributeType openglTypeToVertexAttributeType(uint32_t);
}

using namespace et;

static VertexElement _emptyVertexElement;

VertexElement::VertexElement(VertexAttributeUsage aUsage, VertexAttributeType aType, uint32_t aStride,
	uint32_t aOffset) : _usage(aUsage), _type(aType), _stride(aStride), _offset(aOffset)
{
	/*
	 * Support legacy values
	 */
	if (_type >= VertexAttributeType::max)
		_type = openglTypeToVertexAttributeType(static_cast<uint32_t>(_type));

	_components = vertexAttributeTypeComponents(_type);
	_dataType = vertexAttributeTypeDataType(_type);
}

VertexDeclaration::VertexDeclaration() :
	_interleaved(true), _totalSize(0), _usageMask(0) { }

VertexDeclaration::VertexDeclaration(bool interleaved) :
	_interleaved(interleaved), _totalSize(0), _usageMask(0) { }

VertexDeclaration::VertexDeclaration(bool interleaved, VertexAttributeUsage usage, VertexAttributeType type) : 
	_interleaved(interleaved), _totalSize(0), _usageMask(0) { push_back(usage, type); }

bool VertexDeclaration::has(VertexAttributeUsage usage) const
	{ return (_usageMask & vertexAttributeUsageMask(usage)) != 0; }

bool VertexDeclaration::push_back(VertexAttributeUsage usage, VertexAttributeType type)
	{ return push_back(VertexElement(usage, type, 0, _totalSize)); }

bool VertexDeclaration::push_back(const VertexElement& element)
{
	if (has(element.usage())) return false;

	_usageMask = _usageMask | vertexAttributeUsageMask(element.usage());
	
	_totalSize += vertexAttributeTypeSize(element.type());
	_list.push_back(element);

	if (_interleaved)
	{
		for (auto& i : _list)
			i.setStride(static_cast<int>(_totalSize));
	}

	return true;
}

bool VertexDeclaration::remove(VertexAttributeUsage usage)
{
	auto i = std::find(_list.begin(), _list.end(), usage);
	if (i == _list.end()) return false;

	_list.erase(i);
	_usageMask = _usageMask & (~vertexAttributeUsageMask(usage));

	return true;
}

void VertexDeclaration::clear()
{
	_list.clear();
}

const VertexElement& VertexDeclaration::element(size_t i) const
{
	return (i >= _list.size()) ? _emptyVertexElement : _list[i];
}

const VertexElement& VertexDeclaration::elementForUsage(VertexAttributeUsage u) const
{
	for (auto& e : _list)
	{
		if (e.usage() == u)
			return e;
	};

	return _emptyVertexElement;
}

bool VertexDeclaration::operator == (const VertexDeclaration& r) const
{
	if ((r._interleaved != _interleaved) || (_list.size() != r._list.size())) return false;

	auto si = _list.begin();
	auto ri = r._list.begin();
	while ((si != _list.end()) && (ri != r._list.end()))
	{
		if ((*si) != (*ri))	return false;
		++si;
		++ri;
	}

	return true;
}

void VertexDeclaration::serialize(std::ostream& stream)
{
	serializeUInt32(stream, _interleaved);
	serializeUInt32(stream, _totalSize);
	serializeUInt32(stream, _list.size() & 0xffffffff);
	for (auto& i : _list)
	{
		serializeUInt32(stream, static_cast<uint32_t>(i.usage()));
		serializeUInt32(stream, static_cast<uint32_t>(i.type()));
		serializeUInt32(stream, i.stride());
		serializeUInt32(stream, i.offset());
	}
}

void VertexDeclaration::deserialize(std::istream& stream)
{
	_interleaved = deserializeUInt32(stream) != 0;
	uint32_t totalSize = deserializeUInt32(stream);
	uint32_t listSize = deserializeUInt32(stream);
	for (size_t i = 0; i < listSize; ++i)
	{
		VertexAttributeUsage usage = static_cast<VertexAttributeUsage>(deserializeUInt32(stream));
		VertexAttributeType type = static_cast<VertexAttributeType>(deserializeUInt32(stream));
		uint32_t stride = deserializeUInt32(stream);
		uint32_t offset = deserializeUInt32(stream);
		push_back(VertexElement(usage, type, stride, offset));
	}

	ET_ASSERT(_totalSize == totalSize);
	(void)(totalSize);
}
