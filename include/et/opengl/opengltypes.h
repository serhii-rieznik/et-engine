/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#if !defined(ET_OPENGL_INCLUDES)
#	error Do not include this file directly
#endif

namespace et
{
	enum BlendState
	{
		BlendState_Disabled,
		BlendState_Current,
		BlendState_Default,
		BlendState_AlphaPremultiplied,
		BlendState_Additive,
		BlendState_AlphaAdditive,
		BlendState_AlphaMultiplicative,
		BlendState_ColorAdditive
	};
	
	enum CullState
	{
		CullState_Current,
		CullState_Front,
		CullState_Back
	};
	
	enum DepthFunc
	{
		DepthFunc_Never,
		DepthFunc_Less,
		DepthFunc_LessOrEqual,
		DepthFunc_Equal,
		DepthFunc_GreaterOrEqual,
		DepthFunc_Greater,
		DepthFunc_Always
	};
	
	enum ColorMask
	{
		ColorMask_None = 0x00,
		ColorMask_Red = 0x01,
		ColorMask_Green = 0x02,
		ColorMask_Blue = 0x04,
		ColorMask_Alpha = 0x08,
		ColorMask_RGB = ColorMask_Red | ColorMask_Green | ColorMask_Blue,
		ColorMask_RGBA = ColorMask_RGB | ColorMask_Alpha
	};
	
	enum BufferDrawType
	{
		BufferDrawType_Static,
		BufferDrawType_Dynamic,
		BufferDrawType_Stream,
		BufferDrawType_max
	};

	enum TextureWrap
	{
		TextureWrap_Repeat,
		TextureWrap_ClampToEdge,
		TextureWrap_MirrorRepeat,
		TextureWrap_max
	};

	enum TextureFiltration
	{
		TextureFiltration_Nearest,
		TextureFiltration_Linear,
		TextureFiltration_NearestMipMapNearest,
		TextureFiltration_LinearMipMapNearest,
		TextureFiltration_NearestMipMapLinear,
		TextureFiltration_LinearMipMapLinear,
		TextureFiltration_max
	};
	
	enum PrimitiveType : uint32_t
	{
		PrimitiveType_Points,
		PrimitiveType_Lines,
		PrimitiveType_Triangles,
		PrimitiveType_TriangleStrips,
		PrimitiveType_LineStrip,
		PrimitiveType_max
	};

	enum VertexAttributeUsage : uint32_t
	{
		Usage_Undefined = static_cast<uint32_t>(-1),
		
		Usage_Position = 0,
		Usage_Normal,
		Usage_Color,
		Usage_Tangent,
		Usage_Binormal,
		Usage_TexCoord0,
		Usage_TexCoord1,
		Usage_TexCoord2,
		Usage_TexCoord3,
		Usage_Smoothing,
		
		Usage_InstanceId,
		Usage_InstanceIdExt,

		Usage_max
	};
	
	enum IndexArrayFormat : uint32_t
	{
		IndexArrayFormat_Undefined  = 0,
		IndexArrayFormat_8bit = 1,
		IndexArrayFormat_16bit = 2,
		IndexArrayFormat_32bit = 4,
		IndexArrayFormat_max
	};

	typedef uint32_t IndexType;
	typedef uint16_t ShortIndexType;
	typedef uint8_t SmallIndexType;
	
	enum : uint32_t
	{
		InvalidIndex = static_cast<IndexType>(-1),
		MaxDrawBuffers = 8
	};

}
