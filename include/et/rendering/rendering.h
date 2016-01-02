/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	/*
	 * Common declarations
	 */
	enum class CompareFunction : uint32_t
	{
		Never,
		Less,
		LessOrEqual,
		Equal,
		GreaterOrEqual,
		Greater,
		Always,
		
		max
	};
	
	enum class BlendFunction : uint32_t
	{
		Zero,
		One,
		SourceColor,
		InvSourceColor,
		SourceAlpha,
		InvSourceAlpha,
		DestColor,
		InvDestColor,
		DestAlpha,
		InvDestAlpha,
		
		max
	};
	
	enum class BlendOperation : uint32_t
	{
		Add,
		Subtract,
		ReverseSubtract,
		
		max
	};
	
	enum class CullMode : uint32_t
	{
		Disabled,
		Back,
		Front,
		
		max
	};
	
	enum class FillMode : uint32_t
	{
		Solid,
		Wireframe,
		
		max
	};

	/*
	 * Engine-specific declarations
	 */
	enum class VertexAttributeUsage : uint32_t
	{
		Position,
		Normal,
		Color,
		Tangent,
		Binormal,
		
		TexCoord0,
		TexCoord1,
		TexCoord2,
		TexCoord3,
		
		Smoothing,
		BuiltIn_InstanceId,
		BuiltIn_InstanceIdExt,
		BlendWeights,
		BlendIndices,
		
		BuiltIn_VertexId,
		
		Unknown,
		max
	};
	
	enum class VertexAttributeType : uint32_t
	{
		Float,
		Vec2,
		Vec3,
		Vec4,
		
		Mat3,
		Mat4,
		
		Int,
		IntVec2,
		IntVec3,
		IntVec4,
		
		max
	};
	
	enum class BlendConfiguration : uint32_t
	{
		Disabled,
		Default,
		AlphaPremultiplied,
		Additive,
		AlphaAdditive,
		AlphaMultiplicative,
		ColorAdditive,
		AlphaInverseMultiplicative,
		
		max
	};
	
	enum ColorMask : uint32_t
	{
		None = 0x00,
		Red = 0x01,
		Green = 0x02,
		Blue = 0x04,
		Alpha = 0x08,
		ColorOnly = Red | Green | Blue,
		ColorAndAlpha = ColorOnly | Alpha
	};
	
	enum class BufferDrawType : uint32_t
	{
		Static,
		Dynamic,
		Stream,
		
		max
	};
	
	enum class TextureTarget : uint32_t
	{
		Texture_2D,
		Texture_2D_Array,
		Texture_Rectangle,
		Texture_Cube,
		
		max,
	};
	
	enum class TextureWrap : uint32_t
	{
		Repeat,
		ClampToEdge,
		MirrorRepeat,
		
		max
	};
	
	enum class TextureFormat : uint32_t
	{
		Invalid,
		
		R,
		R8,
		R16,
		R16F,
		R32F,
		
		RG,
		RG8,
		RG16,
		RG16F,
		RG32F,
		
		RGB,
		RGB8,
		RGB16,
		RGB16F,
		RGB32F,
		
		BGR,
		
		RGBA,
		RGBA8,
		RGBA16,
		RGBA16F,
		RGBA32F,
		
		BGRA,
				
		DXT1_RGB,
		DXT1_RGBA,
		DXT3,
		DXT5,
		
		RGTC2,
		
		Depth,
		Depth16,
		Depth24,
		Depth32,
		Depth32F,
		
		PVR_2bpp_RGB,
		PVR_2bpp_sRGB,
		PVR_2bpp_RGBA,
		PVR_2bpp_sRGBA,
		PVR_4bpp_RGB,
		PVR_4bpp_sRGB,
		PVR_4bpp_RGBA,
		PVR_4bpp_sRGBA,

		R11G11B10F,
		
		max
	};
	
	enum class TextureFiltration : uint32_t
	{
		Nearest,
		Linear,
		NearestMipMapNearest,
		LinearMipMapNearest,
		NearestMipMapLinear,
		LinearMipMapLinear,
		
		max
	};
	
	enum class PrimitiveType : uint32_t
	{
		Points,
		Lines,
		Triangles,
		TriangleStrips,
		LineStrips,
		LineStripAdjacency,
		LinesAdjacency,
		max
	};
	
	enum class IndexArrayFormat : uint32_t
	{
		Format_8bit = 1,
		Format_16bit = 2,
		Format_32bit = 4,
		max
	};
	
	enum class DataType : uint32_t
	{
		Char,
		UnsignedChar,
		Short,
		UnsignedShort,
		Int,
		UnsignedInt,
		Half,
		Float,
		Double,
		
		UnsignedShort_4444,
		UnsignedShort_5551,
		UnsignedShort_565,

		UnsignedInt_8888_Rev,
		
		max,
	};
	
	enum class TextureOrigin
	{
		TopLeft,
		BottomLeft,
		
		max
	};
	
	enum class TextureDataLayout
	{
		FacesFirst,
		MipsFirst,
		
		max
	};
	
	enum MapBufferOptions : uint32_t
	{
		Read = 0x01,
		Write = 0x02,
		Unsynchronized = 0x04,
		InvalidateRange = 0x08,
		InvalidateBuffer = 0x10,
	};
	
	enum : uint32_t
	{
		/*
		 * Common values maxes
		 */
		BlendFunction_max = static_cast<uint32_t>(BlendFunction::max),
		BlendOperation_max = static_cast<uint32_t>(BlendOperation::max),
		CompareFunction_max = static_cast<uint32_t>(CompareFunction::max),
		CullMode_max = static_cast<uint32_t>(CullMode::max),
		FillMode_max = static_cast<uint32_t>(FillMode::max),
		
		VertexAttributeUsage_max = static_cast<uint32_t>(VertexAttributeUsage::max),
		VertexAttributeType_max = static_cast<uint32_t>(VertexAttributeType::max),
		IndexArrayFormat_max = static_cast<uint32_t>(IndexArrayFormat::max),
		PrimitiveType_max = static_cast<uint32_t>(PrimitiveType::max),
		BlendConfiguration_max = static_cast<uint32_t>(BlendConfiguration::max),
		DataType_max = static_cast<uint32_t>(DataType::max),
		TextureTarget_max = static_cast<uint32_t>(TextureTarget::max),
		TextureFormat_max = static_cast<uint32_t>(TextureFormat::max),
		InvalidIndex = static_cast<uint32_t>(-1),
		InvalidShortIndex = static_cast<uint16_t>(-1),
		InvalidSmallIndex = static_cast<uint8_t>(-1),
		
		MaxRenderTargets = 8
	};
	
	struct DepthState
	{
		CompareFunction compareFunction = CompareFunction::Less;
		float clearDepth = 1.0f;
		bool depthWriteEnabled = true;
		bool depthTestEnabled = true;
		
		DepthState() = default;
		
		DepthState(bool write, bool test) :
			depthWriteEnabled(write), depthTestEnabled(test) { }
		
		uint32_t sortingKey() const
		{
			return static_cast<uint32_t>(!depthWriteEnabled) << 1 | static_cast<uint32_t>(!depthWriteEnabled);
		}
	};
	
	class BlendState
	{
	public:
		struct Blend
		{
			BlendFunction source = BlendFunction::One;
			BlendFunction dest = BlendFunction::Zero;
			
			Blend() = default;
			Blend(BlendFunction s, BlendFunction d) :
				source(s), dest(d) { }
			
			bool operator == (const Blend& b) const
				{ return (source == b.source) && (dest == b.dest); }
			
			bool operator != (const Blend& b) const
				{ return (source != b.source) || (dest != b.dest); }
		};
		
		BlendState() = default;
		BlendState(uint32_t e) :
			blendEnabled(e) { }
		BlendState(uint32_t e, const Blend& b) :
			blendEnabled(e), color(b), alpha(b) { }
		BlendState(uint32_t e, const Blend& cb, const Blend& ab) :
			blendEnabled(e), color(cb), alpha(ab) { }
		BlendState(uint32_t e, BlendFunction s, BlendFunction d) :
			blendEnabled(e), color(s, d), alpha(s, d) { }
		
		uint32_t sortingKey() const
		{
			return blendEnabled ? 1 : 0;
		}
		
	public:
		Blend color;
		BlendOperation colorOperation = BlendOperation::Add;
		
		Blend alpha;
		BlendOperation alphaOperation = BlendOperation::Add;
		
		bool perRenderTargetBlendEnabled = false;
		bool alphaToCoverageEnabled = false;
		bool blendEnabled = false;
	};
	
	struct RasterizerState
	{
		FillMode fillMode = FillMode::Solid;
		CullMode cullMode = CullMode::Back;
		vec4 clearColor = vec4(0.0f);
		uint32_t colorMask = ColorMask::ColorAndAlpha;
		recti scissorRectangle = recti(0.0f, 0.0f, 0.0f, 0.0f);
		float depthBias = 0.0f;
		float depthSlopeScale = 0.0f;
		bool depthBiasEnabled = false;
		bool scissorEnabled = false;
	};
	
	struct RenderStateCache
	{
		uint32_t activeTextureUnit = 0;
		uint32_t boundFramebuffer = 0;
		uint32_t boundReadFramebuffer = 0;
		uint32_t boundDrawFramebuffer = 0;
		uint32_t boundRenderbuffer = 0;
		uint32_t boundArrayBuffer = 0;
		uint32_t boundElementArrayBuffer = 0;
		uint32_t boundVertexArrayObject = 0;
		uint32_t boundProgram = 0;
		recti viewport = recti(0, 0, 0, 0);
		
		std::map<TextureTarget, std::map<uint32_t, uint32_t>> boundTextures;
		std::array<size_t, VertexAttributeUsage_max> enabledVertexAttributes;
		std::array<size_t, MaxRenderTargets> drawBuffers;
	};
	
	DataType vertexAttributeTypeDataType(VertexAttributeType t);

	VertexAttributeUsage stringToVertexAttributeUsage(const std::string& s, bool& builtIn);
	VertexAttributeType stringToVertexAttributeType(const std::string& s);
	DataType stringToDataType(const std::string&);

	std::string vertexAttributeUsageToString(VertexAttributeUsage);
	std::string vertexAttributeTypeToString(VertexAttributeType);
	std::string dataTypeToString(DataType);

	std::string primitiveTypeToString(PrimitiveType);
	PrimitiveType stringToPrimitiveType(const std::string&);

	std::string indexArrayFormatToString(IndexArrayFormat);
	IndexArrayFormat stringToIndexArrayFormat(const std::string&);

	uint32_t sizeOfDataType(DataType);
	
	uint32_t vertexAttributeUsageMask(VertexAttributeUsage u);
	uint32_t vertexAttributeTypeSize(VertexAttributeType t);
	uint32_t vertexAttributeTypeComponents(VertexAttributeType t);

	uint32_t bitsPerPixelForType(DataType type);
	uint32_t bitsPerPixelForTextureFormat(TextureFormat internalFormat, DataType type);
	uint32_t channelsForTextureFormat(TextureFormat internalFormat);
	
	const std::string& compareFunctionToString(CompareFunction);
	const std::string& blendFunctionToString(BlendFunction);
	const std::string& blendOperationToString(BlendOperation);
	
	CompareFunction stringToCompareFunction(const std::string&);
	BlendFunction stringToBlendFunction(const std::string& );
	BlendOperation stringToBlendOperation(const std::string&);
}
