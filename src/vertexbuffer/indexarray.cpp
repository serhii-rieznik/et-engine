/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/indexarray.h>
#include <et/primitives/primitives.h>

using namespace et;

const uint32_t IndexArray::MaxIndex = 0xffffffff;
const uint16_t IndexArray::MaxShortIndex = 0xffff;
const uint8_t IndexArray::MaxSmallIndex = 0xff;

const int IndexArrayId_1 = ET_COMPOSE_UINT32('I', 'A', 'V', '1');
const int IndexArrayCurrentId = IndexArrayId_1;

static const uint32_t indexTypesMask[static_cast<uint32_t>(IndexArrayFormat::max)] =
{
	0x00000000, // IndexArrayFormat::Undefined = 0
	0x000000ff, // IndexArrayFormat::Format_8bit = 1
	0x0000ffff, // IndexArrayFormat::Format_16bit = 2
	0x00000000, // skip
	0xffffffff, // IndexArrayFormat::Format_32bit = 4
};

size_t verifyDataSize(size_t amount, IndexArrayFormat format);

IndexArray::IndexArray(IndexArrayFormat format, size_t size, PrimitiveType content) : tag(0),
	_data(verifyDataSize(size, format)), _actualSize(0), _format(format), _primitiveType(content)
{
	if (content == PrimitiveType::Points)
		linearize(size);
}

void IndexArray::linearize(size_t indexFrom, size_t indexTo, uint32_t startIndex)
{
	for (size_t i = indexFrom; i < indexTo; ++i)
		setIndex(startIndex++, i);
}

void IndexArray::linearize(size_t size)
{
	linearize(0, size, 0);
}

uint32_t IndexArray::getIndex(size_t pos) const
{
	auto maskValue = indexTypesMask[static_cast<size_t>(_format)];
	
	ET_ASSERT(pos <= maskValue);
	
	return *reinterpret_cast<const uint32_t*>(_data.element_ptr(pos * static_cast<size_t>(_format))) & maskValue;
}

void IndexArray::setIndex(uint32_t value, size_t pos)
{
	unsigned char* elementPtr = _data.element_ptr(pos * static_cast<size_t>(_format));

	if (_format == IndexArrayFormat::Format_32bit)
	{
		uint32_t* ptr = reinterpret_cast<uint32_t*>(elementPtr);
		*ptr = value;
	}
	else if (_format == IndexArrayFormat::Format_16bit)
	{
		ET_ASSERT("Index value out of range" && (value <= MaxShortIndex));
		
		uint16_t* ptr = reinterpret_cast<uint16_t*>(elementPtr);
		*ptr = static_cast<uint16_t>(value);
	}
	else if (_format == IndexArrayFormat::Format_8bit)
	{
		ET_ASSERT("Index value out of range" && (value <= MaxSmallIndex));
		
		uint8_t* ptr = reinterpret_cast<uint8_t*>(elementPtr);
		*ptr = static_cast<uint8_t>(value);
	}

	if (pos + 1 > _actualSize)
		_actualSize = pos + 1;
}

void IndexArray::push_back(uint32_t value)
{
	setIndex(value, _actualSize++);
}

size_t IndexArray::primitivesCount() const
{
	switch (_primitiveType)
	{
		case PrimitiveType::Points:
			return _actualSize;
			
		case PrimitiveType::Lines:
			return _actualSize / 2;
			
		case PrimitiveType::Triangles:
			return _actualSize / 3;
			
		case PrimitiveType::TriangleStrips:
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

IndexArray::PrimitiveIterator IndexArray::primitive(size_t index) const
{
	size_t primitiveIndex = 0;
	switch (_primitiveType)
	{
		case PrimitiveType::Lines:
		{
			primitiveIndex = 2 * index;
			break;
		}
		case PrimitiveType::Triangles:
		{
			primitiveIndex = 3 * index;
			break;
		}
		case PrimitiveType::TriangleStrips:
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
		index[i] = static_cast<size_t>(InvalidIndex);
}

bool IndexArray::Primitive::operator == (const Primitive& p) const
{
	return (p.index[0] == index[0]) && (p.index[1] == index[1]) && (p.index[2] == index[2]);
}

bool IndexArray::Primitive::operator != (const Primitive& p) const
{
	return (p.index[0] != index[0]) || (p.index[1] != index[1]) || (p.index[2] != index[2]);
}

IndexArray::PrimitiveIterator::PrimitiveIterator(const IndexArray* ib, size_t p) :
	_ib(ib), _pos(p)
{
	configure(_pos);
}

void IndexArray::PrimitiveIterator::configure(size_t p)
{
	size_t cap = _ib->_actualSize;
	
	switch (_ib->primitiveType())
	{
		case PrimitiveType::Points:
		{
			_primitive[0] = (p < cap) ? _ib->getIndex(p) : InvalidIndex;
			_primitive[1] = static_cast<uint32_t>(InvalidIndex);
			_primitive[2] = static_cast<uint32_t>(InvalidIndex);
			break;
		}
			
		case PrimitiveType::Lines:
		case PrimitiveType::LineStrip:
		{
			_primitive[0] = (p < cap) ? _ib->getIndex(p) : InvalidIndex; ++p;
			_primitive[1] = (p < cap) ? _ib->getIndex(p) : InvalidIndex;
			_primitive[2] = static_cast<uint32_t>(InvalidIndex);
			break;
		}

		case PrimitiveType::Triangles:
		case PrimitiveType::TriangleStrips:
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
	static const std::map<PrimitiveType, uint32_t> primitiveOffset =
	{
		{ PrimitiveType::Points, 1 },
		{ PrimitiveType::Lines, 2 },
		{ PrimitiveType::Triangles, 3 },
		{ PrimitiveType::TriangleStrips, 1 },
		{ PrimitiveType::LineStrip, 1 },
	};
	
	configure(_pos += primitiveOffset.at(_ib->primitiveType()));
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
	serializeInt(stream, static_cast<uint32_t>(_format));
	serializeInt(stream, static_cast<uint32_t>(_primitiveType));
	serializeInt(stream, static_cast<uint32_t>(_actualSize));
	serializeInt(stream, static_cast<uint32_t>(_data.dataSize()));
	stream.write(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
}

void IndexArray::deserialize(std::istream& stream)
{
	int id = deserializeInt(stream);
	if (id == IndexArrayId_1)
	{
		_format = static_cast<IndexArrayFormat>(deserializeUInt(stream));
		_primitiveType = static_cast<PrimitiveType>(deserializeUInt(stream));
		_actualSize = deserializeUInt(stream);
		_data.resize(deserializeUInt(stream));
		stream.read(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
	}
	else
	{
		ET_FAIL("Unrecognized index array version");
	}
}

/*
 * Service functions
 */
size_t verifyDataSize(size_t amount, IndexArrayFormat format)
{
	return static_cast<size_t>(format) * ((format == IndexArrayFormat::Format_32bit) ? amount : (1 + amount / 4) * 4);
}
