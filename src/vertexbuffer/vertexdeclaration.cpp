/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexdeclaration.h>

namespace et
{
	DataType openglTypeToDataType(uint32_t);
}

using namespace et;

static VertexElement _emptyVertexElement;

VertexElement::VertexElement(VertexAttributeUsage aUsage, DataType aType, uint32_t aStride,
	uint32_t aOffset) : _usage(aUsage), _type(aType), _stride(aStride), _offset(aOffset)
{
	/*
	 * Support legacy values
	 */
	if (_type >= DataType::max)
		_type = openglTypeToDataType(static_cast<uint32_t>(_type));

	_components = dataTypeComponents(_type);
	_dataFormat = dataTypeDataFormat(_type);
}

VertexDeclaration::VertexDeclaration() = default;

VertexDeclaration::VertexDeclaration(bool interleaved) :
	_interleaved(interleaved) { }

VertexDeclaration::VertexDeclaration(bool interleaved, VertexAttributeUsage usage, DataType type) : 
	_interleaved(interleaved) { push_back(usage, type); }

bool VertexDeclaration::has(VertexAttributeUsage usage) const
	{ return (_usageMask & vertexAttributeUsageMask(usage)) != 0; }

bool VertexDeclaration::push_back(VertexAttributeUsage usage, DataType type)
	{ return push_back(VertexElement(usage, type, 0, _totalSize)); }

bool VertexDeclaration::push_back(const VertexElement& element)
{
	if (has(element.usage())) return false;

	_usageMask = _usageMask | vertexAttributeUsageMask(element.usage());
	
	_totalSize += dataTypeSize(element.type());
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

bool VertexDeclaration::hasSameElementsAs(const VertexDeclaration& r) const
{
	if (r.elements().size() != _list.size()) return false;

	for (const auto& ownElement : _list)
	{
		auto usage = ownElement.usage();
		if (!r.has(usage) || (ownElement != r.elementForUsage(usage)))
			return false;
	}

	return true;
}
