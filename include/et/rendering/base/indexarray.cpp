/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/rendering/base/indexarray.h>
#include <et/rendering/base/primitives.h>

namespace et
{

const uint32_t IndexArray::MaxIndex = 0xffffffff;
const uint16_t IndexArray::MaxShortIndex = 0xffff;
const uint8_t IndexArray::MaxSmallIndex = 0xff;

static const uint32_t indexTypeMask[static_cast<uint32_t>(IndexArrayFormat::Count)] =
{
	0x000000ff, // IndexArrayFormat::Format_8bit = 1
	0x0000ffff, // IndexArrayFormat::Format_16bit = 2
	0xffffffff, // IndexArrayFormat::Format_32bit = 4
};

uint32_t verifyDataSize(uint32_t amount, IndexArrayFormat format);

IndexArray::IndexArray(IndexArrayFormat format, uint32_t size, PrimitiveType content) : tag(0),
_data(verifyDataSize(size, format)), _actualSize(0), _format(format), _primitiveType(content)
{
	if (content == PrimitiveType::Points)
		linearize(size);
}

void IndexArray::linearize(uint32_t indexFrom, uint32_t indexTo, uint32_t startIndex)
{
	for (uint32_t i = indexFrom; i < indexTo; ++i)
		setIndex(startIndex++, i);
}

void IndexArray::linearize(uint32_t size)
{
	linearize(0, size, 0);
}

uint32_t IndexArray::getIndex(uint32_t pos) const
{
	auto maskValue = indexTypeMask[static_cast<uint32_t>(_format) / 2];

	ET_ASSERT(pos <= maskValue);

	return *reinterpret_cast<const uint32_t*>(_data.element_ptr(pos * static_cast<uint32_t>(_format))) & maskValue;
}

void IndexArray::setIndex(uint32_t value, uint32_t pos)
{
	unsigned char* elementPtr = _data.element_ptr(pos * static_cast<uint32_t>(_format));

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

uint32_t IndexArray::primitivesCount() const
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

void IndexArray::resize(uint32_t count)
{
	_actualSize = std::min(_actualSize, count);
	_data.resize(verifyDataSize(count, _format));
}

void IndexArray::resizeToFit(uint32_t count)
{
	_actualSize = std::min(_actualSize, count);
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

IndexArray::PrimitiveIterator IndexArray::primitive(uint32_t index) const
{
	uint32_t primitiveIndex = 0;
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

/*
 * Supporting types
 */
IndexArray::Primitive::Primitive()
{
	for (uint32_t i = 0; i < IndexArray::Primitive::VertexCount_max; ++i)
		index[i] = static_cast<uint32_t>(InvalidIndex);
}

bool IndexArray::Primitive::operator == (const Primitive& p) const
{
	return (p.index[0] == index[0]) && (p.index[1] == index[1]) && (p.index[2] == index[2]);
}

bool IndexArray::Primitive::operator != (const Primitive& p) const
{
	return (p.index[0] != index[0]) || (p.index[1] != index[1]) || (p.index[2] != index[2]);
}

IndexArray::PrimitiveIterator::PrimitiveIterator(const IndexArray* ib, uint32_t p) :
	_ib(ib), _pos(p)
{
	configure(_pos);
}

void IndexArray::PrimitiveIterator::configure(uint32_t p)
{
	uint32_t cap = _ib->_actualSize;

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
	case PrimitiveType::LineStrips:
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

IndexArray::PrimitiveIterator& IndexArray::PrimitiveIterator::operator ++() {

	static const uint32_t offsets[uint32_t(PrimitiveType::max)] =
	{
		1, // Points,
		2, // Lines,
		3, // Triangles,
		1, // TriangleStrips,
		1, // LineStrips,
		1, // LineStripAdjacency,
		1, // LinesAdjacency,
	};
	configure(_pos += offsets[uint32_t(_ib->primitiveType())]);
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

void IndexArray::serialize(std::ostream&)
{

}

void IndexArray::deserialize(std::istream&)
{

}


/*
 * Service functions
 */
uint32_t verifyDataSize(uint32_t amount, IndexArrayFormat format)
{
	return static_cast<uint32_t>(format) * ((format == IndexArrayFormat::Format_32bit) ? amount : (1 + amount / 4) * 4);
}

}
