/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/indexarray.h>
#include <et/primitives/primitives.h>

using namespace et;

const IndexType IndexArray::MaxShortIndex = 65536;
const IndexType IndexArray::MaxSmallIndex = 256;

const int IndexArrayId_1 = ET_COMPOSE_UINT32('I', 'A', 'V', '1');
const int IndexArrayCurrentId = IndexArrayId_1;

static const IndexType indexTypeMask[IndexArrayFormat_max] = {
	0x00000000, // IndexArrayFormat_Undefined = 0
	0x000000ff, // IndexArrayFormat_8bit = 1
	0x0000ffff, // IndexArrayFormat_16bit = 2
	0x00000000, // skip
	0xffffffff, // IndexArrayFormat_32bit = 4
};

size_t verifyDataSize(size_t amount, IndexArrayFormat format)
{
	return format * ((format == IndexArrayFormat_32bit) ? amount : (1 + amount / 4) * 4);
}

IndexArray::IndexArray(IndexArrayFormat format, size_t size, PrimitiveType content) : tag(0),
	_data(verifyDataSize(size, format)), _actualSize(0), _format(format), _primitiveType(content)
{
	if (content == PrimitiveType_Points)
		linearize(size);
}

void IndexArray::linearize(size_t indexFrom, size_t indexTo, IndexType startIndex)
{
	for (size_t i = indexFrom; i < indexTo; ++i)
		setIndex(startIndex++, i);
}

void IndexArray::linearize(size_t size)
{
	linearize(0, size, 0);
}

IndexType IndexArray::getIndex(size_t pos) const
{
	ET_ASSERT(pos <= indexTypeMask[_format]);
	return *reinterpret_cast<const IndexType*>(_data.element_ptr(pos * _format)) & indexTypeMask[_format];
}

void IndexArray::setIndex(IndexType value, size_t pos)
{
	unsigned char* elementPtr = _data.element_ptr(pos * _format);

	if (_format == IndexArrayFormat_32bit)
	{
		IndexType* ptr = reinterpret_cast<IndexType*>(elementPtr);
		*ptr = value;
	}
	else if (_format == IndexArrayFormat_16bit)
	{
		ET_ASSERT("Index value out of range" && (value < MaxShortIndex));
		ShortIndexType* ptr = reinterpret_cast<ShortIndexType*>(elementPtr);
		*ptr = static_cast<ShortIndexType>(value);
	}
	else if (_format == IndexArrayFormat_8bit)
	{
		ET_ASSERT("Index value out of range" && (value < MaxSmallIndex));
		SmallIndexType* ptr = reinterpret_cast<SmallIndexType*>(elementPtr);
		*ptr = static_cast<SmallIndexType>(value);
	}

	if (pos + 1 > _actualSize)
		_actualSize = pos + 1;
}

void IndexArray::push_back(IndexType value)
{
	setIndex(value, _actualSize++);
}

size_t IndexArray::primitivesCount() const
{
	switch (_primitiveType)
	{
		case PrimitiveType_Points:
			return _actualSize;
			
		case PrimitiveType_Lines:
			return _actualSize / 2;
			
		case PrimitiveType_Triangles:
			return _actualSize / 3;
			
		case PrimitiveType_TriangleStrips:
			return _actualSize - 2;
			
		default:
			break;
	}
	
	return 0;
}

void IndexArray::resize(size_t count)
{
	_actualSize = etMin(_actualSize, count);
	_data.resize(verifyDataSize(count, _format));
}

void IndexArray::resizeToFit(size_t count)
{
	_actualSize = etMin(_actualSize, count);
	_data.fitToSize(verifyDataSize(count, _format));
}

void IndexArray::compact()
{
	_data.resize(verifyDataSize(_actualSize, _format));
}

IndexArray::PrimitiveIterator IndexArray::begin() const
{
	return IndexArray::PrimitiveIterator(this, 0);
}

IndexArray::PrimitiveIterator IndexArray::end() const
{
	return IndexArray::PrimitiveIterator(this, capacity());
}

IndexArray::PrimitiveIterator IndexArray::primitive(IndexType index) const
{
	IndexType primitiveIndex = 0;
	switch (_primitiveType)
	{
		case PrimitiveType_Lines:
		{
			primitiveIndex = 2 * index;
			break;
		}
		case PrimitiveType_Triangles:
		{
			primitiveIndex = 3 * index;
			break;
		}
		case PrimitiveType_TriangleStrips:
		{
			primitiveIndex = index == 0 ? 0 : (2 + index);
			break;
		}
			
		default:
			primitiveIndex = index;
	}

	return IndexArray::PrimitiveIterator(this, primitiveIndex > capacity() ? capacity() : primitiveIndex);
}

/**
 *
 * Supporting types
 *
 */

IndexArray::Primitive::Primitive()
{
	for (size_t i = 0; i < IndexArray::Primitive::VertexCount_max; ++i)
		index[i] = InvalidIndex;
}

bool IndexArray::Primitive::operator == (const Primitive& p) const
{
	return (p.index[0] == index[0]) && (p.index[1] == index[1]) && (p.index[2] == index[2]);
}

bool IndexArray::Primitive::operator != (const Primitive& p) const
{
	return (p.index[0] != index[0]) || (p.index[1] != index[1]) || (p.index[2] != index[2]);
}

IndexArray::PrimitiveIterator::PrimitiveIterator(const IndexArray* ib, IndexType p) :
	_ib(ib), _pos(p)
{
	configure(_pos);
}

void IndexArray::PrimitiveIterator::configure(IndexType p)
{
	IndexType cap = static_cast<IndexType>(_ib->_actualSize);
	
	switch (_ib->primitiveType())
	{
		case PrimitiveType_Points:
		{
			_primitive[0] = (p < cap) ? _ib->getIndex(p) : InvalidIndex;
			_primitive[1] = InvalidIndex;
			_primitive[2] = InvalidIndex;
			break;
		}
			
		case PrimitiveType_Lines:
		case PrimitiveType_LineStrip:
		{
			_primitive[0] = (p < cap) ? _ib->getIndex(p) : InvalidIndex; ++p;
			_primitive[1] = (p < cap) ? _ib->getIndex(p) : InvalidIndex;
			_primitive[2] = InvalidIndex;
			break;
		}

		case PrimitiveType_Triangles:
		case PrimitiveType_TriangleStrips:
		{
			_primitive[0] = (p < cap) ? _ib->getIndex(p) : InvalidIndex; ++p;
			_primitive[1] = (p < cap) ? _ib->getIndex(p) : InvalidIndex; ++p;
			_primitive[2] = (p < cap) ? _ib->getIndex(p) : InvalidIndex;
			break;
		}

		default:
			ET_FAIL("Unsupported PrimitiveType value");
	}
}

IndexArray::PrimitiveIterator& IndexArray::PrimitiveIterator::operator = (const PrimitiveIterator& p)
{
	_ib = p._ib;
	_pos = p._pos;
	_primitive = p._primitive;
	return *this;
}

IndexArray::PrimitiveIterator& IndexArray::PrimitiveIterator::operator ++()
{
	static const IndexType primitiveOffset[PrimitiveType_max] =
	{
		1, // PrimitiveType_Points,
		1, // PrimitiveType_Lines,
		3, // PrimitiveType_Triangles,
		1, // PrimitiveType_TriangleStrips,
		1, // PrimitiveType_LineStrip,
	};
	configure(_pos += primitiveOffset[_ib->primitiveType()]);
	return *this;
}

IndexArray::PrimitiveIterator IndexArray::PrimitiveIterator::operator ++(int)
{
	IndexArray::PrimitiveIterator temp = *this;
	++(*this);
	return temp;
}

bool IndexArray::PrimitiveIterator::operator == (const IndexArray::PrimitiveIterator& p) const
{
	return _primitive == p._primitive;
}

bool IndexArray::PrimitiveIterator::operator != (const IndexArray::PrimitiveIterator& p) const
{
	return _primitive != p._primitive;
}

void IndexArray::serialize(std::ostream& stream)
{
	serializeInt(stream, IndexArrayCurrentId);
	serializeInt(stream, _format);
	serializeInt(stream, _primitiveType);
	serializeInt(stream, static_cast<int>(_actualSize));
	serializeInt(stream, static_cast<int>(_data.dataSize()));
	stream.write(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
}

void IndexArray::deserialize(std::istream& stream)
{
	int id = deserializeInt(stream);
	if (id == IndexArrayId_1)
	{
		_format = static_cast<IndexArrayFormat>(deserializeInt(stream));
		_primitiveType = static_cast<PrimitiveType>(deserializeInt(stream));
		_actualSize = deserializeUInt(stream);
		_data.resize(deserializeUInt(stream));
		stream.read(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
	}
	else
	{
		ET_FAIL("Unrecognized index array version");
	}
}
