/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{

static VertexElement _emptyVertexElement;

VertexElement::VertexElement(VertexAttributeUsage aUsage, DataType aType, uint32_t aStride,
	uint32_t aOffset) : _usage(aUsage), _type(aType), _stride(aStride), _offset(aOffset)
{
	ET_ASSERT(_type < DataType::max);

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
	{ return push_back(VertexElement(usage, type, 0, _size)); }

bool VertexDeclaration::push_back(const VertexElement& el)
{
	if (has(el.usage()))
		return false;

	_usageMask = _usageMask | vertexAttributeUsageMask(el.usage());
	
	_size += dataTypeSize(el.type());
	_elements.insert(el);

	if (_interleaved)
	{
		VertexElementSet rebuild = _elements;
		
		_elements.clear();
		for (VertexElement e : rebuild)
		{
			e.setStride(_size);
			_elements.insert(e);
		}
	}

	return true;
}

bool VertexDeclaration::remove(VertexAttributeUsage usage)
{
	auto i = std::find(_elements.begin(), _elements.end(), usage);
	if (i == _elements.end())
		return false;

	_elements.erase(i);
	_usageMask = _usageMask & (~vertexAttributeUsageMask(usage));

	return true;
}

void VertexDeclaration::clear()
{
	_elements.clear();
}

const VertexElement& VertexDeclaration::element(uint32_t i) const
{
	ET_ASSERT(i < _elements.size());
	auto it = _elements.begin();
	std::advance(it, i);
	return (*it);
}

const VertexElement& VertexDeclaration::elementForUsage(VertexAttributeUsage u) const
{
	for (auto& e : _elements)
	{
		if (e.usage() == u)
			return e;
	};

	return _emptyVertexElement;
}

bool VertexDeclaration::operator == (const VertexDeclaration& r) const
{
	if ((r._interleaved != _interleaved) || (_elements.size() != r._elements.size()))
		return false;

	auto si = _elements.begin();
	auto ri = r._elements.begin();
	while ((si != _elements.end()) && (ri != r._elements.end()))
	{
		if ((*si) != (*ri))
			return false;

		++si;
		++ri;
	}

	return true;
}

bool VertexDeclaration::hasSameElementsAs(const VertexDeclaration& r) const
{
	if (r.elements().size() != _elements.size())
		return false;

	for (const auto& ownElement : _elements)
	{
		auto usage = ownElement.usage();
		if (!r.has(usage) || (ownElement != r.elementForUsage(usage)))
			return false;
	}

	return true;
}

void VertexDeclaration::serialize(std::ostream& fOut)
{
	serializeUInt32(fOut, 0);
	serializeUInt32(fOut, static_cast<uint32_t>(_elements.size()));
	for (const VertexElement& e : _elements)
	{
		serializeUInt32(fOut, static_cast<uint32_t>(e.usage()));
		serializeUInt32(fOut, static_cast<uint32_t>(e.type()));
	}
}

void VertexDeclaration::deserialize(std::istream& fIn)
{
	clear();

	/* uint32_t version = */ deserializeUInt32(fIn);
	uint32_t elementCount = deserializeUInt32(fIn);
	for (uint32_t i = 0; i < elementCount; ++i)
	{
		VertexAttributeUsage usage = static_cast<VertexAttributeUsage>(deserializeUInt32(fIn));
		DataType type = static_cast<DataType>(deserializeUInt32(fIn));
		push_back(usage, type);
	}
}

}
