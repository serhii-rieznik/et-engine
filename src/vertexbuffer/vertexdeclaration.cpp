/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <algorithm>
#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexdeclaration.h>

using namespace et;

VertexElement VertexDeclaration::_empty;

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
	return (i >= _list.size()) ? _empty : _list[i];
}

const VertexElement& VertexDeclaration::elementForUsage(VertexAttributeUsage u) const
{
	for (auto& e : _list)
	{
		if (e.usage() == u)
			return e;
	};

	return _empty;
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
	serializeInt(stream, _interleaved);
	serializeInt(stream, static_cast<int>(_totalSize));
	serializeInt(stream, static_cast<int>(_list.size()));
	for (auto& i : _list)
	{
		serializeInt(stream, i.usage());
		serializeInt(stream, i.type());
		serializeInt(stream, i.stride());
		serializeInt(stream, static_cast<int>(i.offset()));
	}
}

void VertexDeclaration::deserialize(std::istream& stream)
{
	_interleaved = deserializeInt(stream) != 0;
	size_t totalSize = static_cast<size_t>(deserializeInt(stream));
	size_t listSize = deserializeUInt(stream);
	for (size_t i = 0; i < listSize; ++i)
	{
		VertexAttributeUsage usage = static_cast<VertexAttributeUsage>(deserializeInt(stream));
		VertexAttributeType type = static_cast<VertexAttributeType>(deserializeInt(stream));
		int stride = deserializeInt(stream);
		size_t offset = deserializeUInt(stream);
		push_back(VertexElement(usage, type, stride, offset));
	}

	ET_ASSERT(_totalSize == totalSize);
	(void)(totalSize);
}
