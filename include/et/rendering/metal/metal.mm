/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>

namespace et
{
namespace metal
{

MTLTextureType textureTargetValue(TextureTarget value, uint32_t samples)
{
    static const std::map<TextureTarget, MTLTextureType> _map =
    {
        {TextureTarget::Texture_2D, MTLTextureType2D},
        {TextureTarget::Texture_Cube, MTLTextureTypeCube},
        {TextureTarget::Texture_2D_Array, MTLTextureType2DArray},
        {TextureTarget::Texture_Rectangle, MTLTextureType(-1)},
    };
    
    if (samples > 1)
    {
        ET_ASSERT(value == TextureTarget::Texture_2D);
        return MTLTextureType2DMultisample;
    }
    
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}
    
MTLPixelFormat textureFormatValue(TextureFormat value)
{
    static const std::map<TextureFormat, MTLPixelFormat> _map =
    {
        {TextureFormat::RGBA8, MTLPixelFormatRGBA8Unorm},
        {TextureFormat::RGBA16F, MTLPixelFormatRGBA16Float},
        {TextureFormat::RGBA32F, MTLPixelFormatRGBA32Float},
        // TODO : add more
    };
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}

MTLPixelFormat renderableTextureFormatValue(TextureFormat value)
{
	static const std::map<TextureFormat, MTLPixelFormat> _map =
	{
		{TextureFormat::RGBA8, MTLPixelFormatBGRA8Unorm},
		// TODO : add more
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

MTLPrimitiveType primitiveTypeValue(PrimitiveType value)
{
    static const std::map<PrimitiveType, MTLPrimitiveType> _map =
    {
        { PrimitiveType::Points, MTLPrimitiveTypePoint },
        { PrimitiveType::Lines, MTLPrimitiveTypeLine },
        { PrimitiveType::LineStrips, MTLPrimitiveTypeLineStrip },
        { PrimitiveType::Triangles, MTLPrimitiveTypeTriangle },
        { PrimitiveType::TriangleStrips, MTLPrimitiveTypeTriangleStrip },
    };
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}

MTLIndexType indexArrayFormat(IndexArrayFormat value)
{
	static const std::map<IndexArrayFormat, MTLIndexType> _map =
	{
		{ IndexArrayFormat::Format_16bit, MTLIndexTypeUInt16 },
		{ IndexArrayFormat::Format_32bit, MTLIndexTypeUInt32 },
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

MTLPrimitiveTopologyClass primitiveTypeToTopology(PrimitiveType pt)
{
    switch (pt)
    {
        case PrimitiveType::Points:
            return MTLPrimitiveTopologyClassPoint;
        case PrimitiveType::Lines:
        case PrimitiveType::LineStrips:
            return MTLPrimitiveTopologyClassLine;
        case PrimitiveType::Triangles:
        case PrimitiveType::TriangleStrips:
            return MTLPrimitiveTopologyClassTriangle;
        default:
            ET_FAIL("Unsupported primitive type");
            return MTLPrimitiveTopologyClassPoint;
    }
}

MTLCompareFunction compareFunctionValue(CompareFunction func)
{
	switch (func)
	{
		case CompareFunction::Never:
			return MTLCompareFunctionNever;
		case CompareFunction::Less:
			return MTLCompareFunctionLess;
		case CompareFunction::LessOrEqual:
			return MTLCompareFunctionLessEqual;
		case CompareFunction::Equal:
			return MTLCompareFunctionEqual;
		case CompareFunction::GreaterOrEqual:
			return MTLCompareFunctionGreaterEqual;
		case CompareFunction::Greater:
			return MTLCompareFunctionGreater;
		case CompareFunction::Always:
			return MTLCompareFunctionAlways;
		default:
			ET_FAIL("Invalid CompareFunction specified.");
	}
}

MTLVertexFormat dataTypeToVertexFormat(DataType value)
{
    static const std::map<DataType, MTLVertexFormat> _map =
    {
        { DataType::Int, MTLVertexFormatInt },
        { DataType::IntVec2, MTLVertexFormatInt2 },
        { DataType::IntVec3, MTLVertexFormatInt3 },
        { DataType::IntVec4, MTLVertexFormatInt4 },
        { DataType::Float, MTLVertexFormatFloat },
        { DataType::Vec2, MTLVertexFormatFloat2 },
        { DataType::Vec3, MTLVertexFormatFloat3 },
        { DataType::Vec4, MTLVertexFormatFloat4 },
    };
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}

DataType mtlDataTypeToDataType(MTLDataType value)
{
	static const std::map<MTLDataType, DataType> _map =
	{
		{ MTLDataTypeFloat, DataType::Float },
		{ MTLDataTypeFloat2, DataType::Vec2 },
		{ MTLDataTypeFloat3, DataType::Vec3 },
		{ MTLDataTypeFloat4, DataType::Vec4 },
		{ MTLDataTypeInt, DataType::Int },
		{ MTLDataTypeInt2, DataType::IntVec2 },
		{ MTLDataTypeInt3, DataType::IntVec3 },
		{ MTLDataTypeInt4, DataType::IntVec4 },
		{ MTLDataTypeFloat3x3, DataType::Mat3 },
		{ MTLDataTypeFloat4x4, DataType::Mat4 },
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

MTLSamplerAddressMode wrapModeToAddressMode(TextureWrap value)
{
	static const std::map<TextureWrap, MTLSamplerAddressMode> _map =
	{
		{ TextureWrap::Repeat, MTLSamplerAddressModeRepeat },
		{ TextureWrap::ClampToEdge, MTLSamplerAddressModeClampToEdge },
		{ TextureWrap::MirrorRepeat, MTLSamplerAddressModeMirrorRepeat },
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

MTLSamplerMipFilter textureFilteringToMipFilter(TextureFiltration value)
{
	static const std::map<TextureFiltration, MTLSamplerMipFilter> _map =
	{
		{ TextureFiltration::Nearest, MTLSamplerMipFilterNearest },
		{ TextureFiltration::Linear, MTLSamplerMipFilterLinear },
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

MTLSamplerMinMagFilter textureFilteringToSamplerFilter(TextureFiltration value)
{
	static const std::map<TextureFiltration, MTLSamplerMinMagFilter> _map =
	{
		{ TextureFiltration::Nearest, MTLSamplerMinMagFilterNearest },
		{ TextureFiltration::Linear, MTLSamplerMinMagFilterLinear },
	};
	ET_ASSERT(_map.count(value) > 0);
	return _map.at(value);
}

}
}
