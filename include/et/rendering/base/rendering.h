/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{

enum class RenderingAPI : uint32_t
{
	Metal,
	Vulkan,
	
	Count
};

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

enum class RenderPassClass : uint32_t
{
	Forward,
	Depth
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

	BlendWeights,
	BlendIndices,

	Unknown,
	max
};

enum class DataType : uint32_t
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
	AlphaBlend,
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
	R8,
	R16,		
	R16F,
	R32F,
	RG8,
	RG16,
	RG16F,
	RG32F,
	RGB565,
	RGBA8,
	BGRA8,
	RGBA16,
	RGBA16F,
	RGBA32F,
	DXT1_RGB,
	DXT1_RGBA,
	DXT3,
	DXT5,
	RGTC2,
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
	Count = 3
};

enum class DataFormat : uint32_t
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

enum class TextureOrigin : uint32_t
{
	TopLeft,
	BottomLeft,
	
	max
};

enum class TextureDataLayout : uint32_t
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
	DataType_max = static_cast<uint32_t>(DataType::max),
	PrimitiveType_max = static_cast<uint32_t>(PrimitiveType::max),
	BlendConfiguration_max = static_cast<uint32_t>(BlendConfiguration::max),
	DataFormat_max = static_cast<uint32_t>(DataFormat::max),
	TextureTarget_max = static_cast<uint32_t>(TextureTarget::max),
	TextureFormat_max = static_cast<uint32_t>(TextureFormat::max),

	/*
	 * Invalid indices
	 */
	InvalidIndex = static_cast<uint32_t>(-1),
	InvalidShortIndex = static_cast<uint16_t>(-1),
	InvalidSmallIndex = static_cast<uint8_t>(-1),

	/*
	 * Shader buffer indices
	 */
	VertexStreamBufferIndex = 0,
	ObjectVariablesBufferIndex = 4,
	MaterialVariablesBufferIndex = 5,
	PassVariablesBufferIndex = 6,

	MaxRenderTargets = 8,
	MaxTextureUnits = 8
};

struct DepthState
{
	CompareFunction compareFunction = CompareFunction::Less;
	bool depthWriteEnabled = true;
	bool depthTestEnabled = false;
	
	DepthState() = default;
	
	DepthState(bool write, CompareFunction func) :
		compareFunction(func), depthWriteEnabled(write) { }
	
	uint32_t sortingKey() const
	{
		return static_cast<uint32_t>(depthWriteEnabled) << 1 | static_cast<uint32_t>(compareFunction);
	}

	bool operator == (const DepthState& r) const
	{
		return (compareFunction == r.compareFunction) && (depthWriteEnabled == r.depthWriteEnabled);
	}

	bool operator != (const DepthState& r) const
	{
		return (compareFunction != r.compareFunction) || (depthWriteEnabled != r.depthWriteEnabled);
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
	BlendState(bool e) :
		enabled(e) { }
	BlendState(bool e, const Blend& b) :
		color(b), alpha(b), enabled(e) { }
	BlendState(bool e, const Blend& cb, const Blend& ab) :
		color(cb), alpha(ab), enabled(e) { }
	BlendState(bool e, BlendFunction s, BlendFunction d) :
		color(s, d), alpha(s, d), enabled(e) { }
	
	uint32_t sortingKey() const
		{ return enabled ? 0 : 1; }
	
	bool operator == (const BlendState& bs) const
	{
		return (color == bs.color) && (enabled == bs.enabled) && (alpha == bs.alpha) &&
			(alphaToCoverageEnabled == bs.alphaToCoverageEnabled) &&
			(perRenderTargetBlendEnabled == bs.perRenderTargetBlendEnabled) &&
			(colorOperation == bs.colorOperation) && (alphaOperation == bs.alphaOperation);
	}

	bool operator != (const BlendState& bs) const
	{
		return (color != bs.color) || (enabled != bs.enabled) || (alpha != bs.alpha) ||
			(alphaToCoverageEnabled != bs.alphaToCoverageEnabled) ||
			(perRenderTargetBlendEnabled != bs.perRenderTargetBlendEnabled) ||
			(colorOperation != bs.colorOperation) || (alphaOperation != bs.alphaOperation);
	}

public:
	Blend color;
	BlendOperation colorOperation = BlendOperation::Add;
	
	Blend alpha;
	BlendOperation alphaOperation = BlendOperation::Add;
	
	bool perRenderTargetBlendEnabled = false;
	bool alphaToCoverageEnabled = false;
	bool enabled = false;
};

struct RasterizerState
{
	FillMode fillMode = FillMode::Solid;
	CullMode cullMode = CullMode::Back;
	vec4 clearColor = vec4(0.0f);
	uint32_t colorMask = ColorMask::ColorAndAlpha;
	recti scissorRectangle = recti(0, 0, 0, 0);
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
	uint32_t boundVertexStream = 0;
	uint32_t boundProgram = 0;
	recti viewport = recti(0, 0, 0, 0);
	
	std::array<std::array<uint32_t, MaxTextureUnits>, TextureTarget_max> boundTextures;
	std::array<size_t, VertexAttributeUsage_max> enabledVertexAttributes;
	std::array<size_t, MaxRenderTargets> drawBuffers;
};

enum class FramebufferOperation : uint32_t
{
	DontCare,
	Clear,
	Load,
	Store,
	Discard,
	MultisampleResolve
};

DataFormat dataTypeDataFormat(DataType t);

VertexAttributeUsage stringToVertexAttributeUsage(const std::string& s);
DataType stringToDataType(const std::string& s, RenderingAPI);
DataFormat stringToDataFormat(const std::string&);

std::string vertexAttributeUsageToString(VertexAttributeUsage);
std::string vertexAttributeUsageSemantics(VertexAttributeUsage);
std::string dataTypeToString(DataType, RenderingAPI);
std::string dataFormatToString(DataFormat);

std::string primitiveTypeToString(PrimitiveType);
PrimitiveType stringToPrimitiveType(const std::string&);

std::string indexArrayFormatToString(IndexArrayFormat);
IndexArrayFormat stringToIndexArrayFormat(const std::string&);
DataFormat indexArrayFormatToDataFormat(IndexArrayFormat);

uint32_t sizeOfDataFormat(DataFormat);

uint32_t vertexAttributeUsageMask(VertexAttributeUsage u);
uint32_t dataTypeSize(DataType t);
uint32_t dataTypeComponents(DataType t);

uint32_t bitsPerPixelForDataFormat(DataFormat type);
uint32_t bitsPerPixelForTextureFormat(TextureFormat internalFormat);
uint32_t channelsForTextureFormat(TextureFormat internalFormat);
bool isCompressedTextureFormat(TextureFormat internalFormat);
bool isDepthTextureFormat(TextureFormat internalFormat);

const std::string& compareFunctionToString(CompareFunction);
const std::string& blendFunctionToString(BlendFunction);
const std::string& blendOperationToString(BlendOperation);

CompareFunction stringToCompareFunction(const std::string&);
BlendFunction stringToBlendFunction(const std::string& );
BlendOperation stringToBlendOperation(const std::string&);

BlendState blendConfigurationToBlendState(BlendConfiguration);
std::string blendConfigurationToString(BlendConfiguration);
bool blendStateToConfiguration(const BlendState&, BlendConfiguration&);
bool stringToBlendConfiguration(const std::string& name, BlendConfiguration& config);

Dictionary serializeDepthState(const DepthState&);
Dictionary serializeBlendState(const BlendState&);
DepthState deserializeDepthState(const Dictionary&);
BlendState deserializeBlendState(const Dictionary&);

std::string cullModeToString(CullMode);
bool stringToCullMode(const std::string&, CullMode&);

RenderPassClass stringToRenderPassClass(const std::string&);

template <class T>
DataType dataTypeFromClass();

template <> inline DataType dataTypeFromClass<float>() { return DataType::Float; }
template <> inline DataType dataTypeFromClass<vec2>() { return DataType::Vec2; }
template <> inline DataType dataTypeFromClass<vec3>() { return DataType::Vec3; }
template <> inline DataType dataTypeFromClass<vec4>() { return DataType::Vec4; }
template <> inline DataType dataTypeFromClass<mat3>() { return DataType::Mat3; }
template <> inline DataType dataTypeFromClass<mat4>() { return DataType::Mat4; }
template <> inline DataType dataTypeFromClass<int32_t>() { return DataType::Int; }
template <> inline DataType dataTypeFromClass<uint32_t>() { return DataType::Int; }
template <> inline DataType dataTypeFromClass<int64_t>() { return DataType::Int; }
template <> inline DataType dataTypeFromClass<uint64_t>() { return DataType::Int; }
template <> inline DataType dataTypeFromClass<vec2i>() { return DataType::IntVec2; }
template <> inline DataType dataTypeFromClass<vec3i>() { return DataType::IntVec3; }
template <> inline DataType dataTypeFromClass<vec4i>() { return DataType::IntVec4; }

template <class T>
const char* classToString();

#define ET_CLASS_TO_STR_IMPL(CLS) template <> inline const char* classToString<CLS>() { return #CLS; }
	ET_CLASS_TO_STR_IMPL(float)
	ET_CLASS_TO_STR_IMPL(vec2)
	ET_CLASS_TO_STR_IMPL(vec3)
	ET_CLASS_TO_STR_IMPL(vec4)
	ET_CLASS_TO_STR_IMPL(int32_t)
	ET_CLASS_TO_STR_IMPL(int64_t)
	ET_CLASS_TO_STR_IMPL(uint32_t)
	ET_CLASS_TO_STR_IMPL(uint64_t)
	ET_CLASS_TO_STR_IMPL(vec2i)
	ET_CLASS_TO_STR_IMPL(vec3i)
	ET_CLASS_TO_STR_IMPL(vec4i)
	ET_CLASS_TO_STR_IMPL(mat3)
	ET_CLASS_TO_STR_IMPL(mat4)
#undef ET_CLASS_TO_STR_IMPL
}
