/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/indexarray.h>

namespace et
{
	class IndexBuffer : public Object
	{
	public:
		ET_DECLARE_POINTER(IndexBuffer);
		
	public:
        IndexBuffer(IndexArray::Pointer i, BufferDrawType drawType, const std::string& name)
			: Object(name)
            , _drawType(drawType)
            , _primitiveType(i->primitiveType())
            , _format(i->format())
            , _size(i->actualSize())
        {
            switch (_format)
            {
                case IndexArrayFormat::Format_8bit:
                {
                    _dataFormat = DataFormat::UnsignedChar;
                    break;
                }
                case IndexArrayFormat::Format_16bit:
                {
                    _dataFormat = DataFormat::UnsignedShort;
                    break;
                }
                case IndexArrayFormat::Format_32bit:
                {
                    _dataFormat = DataFormat::UnsignedInt;
                    break;
                }
                default:
                    ET_FAIL_FMT("Invalid IndexArrayFormat value: %u", static_cast<uint32_t>(_format));
            }
        }
        
		~IndexBuffer() = default;

        uint32_t size() const
            { return _size; }
        
        void setSize(uint32_t sz)
            { _size = sz; }
        
		BufferDrawType drawType() const
			{ return _drawType; }

        IndexArrayFormat format() const
            { return _format; }
        
        DataFormat dataFormat() const
            { return _dataFormat; }
        
        PrimitiveType primitiveType() const
            { return _primitiveType; }
        
        void overridePrimitiveType(PrimitiveType pt)
            { _primitiveType = pt; }
        
		const std::string& sourceObjectName() const
			{ return _sourceObjectName; }

		void setSourceObjectName(const std::string& tag)
			{ _sourceObjectName = tag; }

        virtual void bind() = 0;
        virtual void clear() = 0;
        
	private:
		std::string _sourceObjectName;
        BufferDrawType _drawType = BufferDrawType::Static;
        PrimitiveType _primitiveType = PrimitiveType::Triangles;
        IndexArrayFormat _format = IndexArrayFormat::Format_16bit;
        DataFormat _dataFormat = DataFormat::UnsignedShort;
        uint32_t _size = 0;
	};
}
