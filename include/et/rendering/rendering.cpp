/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendering.h>

using namespace et;

const std::string vertexAttributeUsageNames[VertexAttributeUsage_max] =
{
	"Vertex", "Normal", "Color", "Tangent", "Binormal",
	"TexCoord0", "TexCoord1", "TexCoord2", "TexCoord3",
	"SmoothingGroup", "gl_InstanceID", "gl_InstanceIDEXT",
	"BlendWeights", "BlendIndices", "gl_VertexID"
};

const std::string dataTypeNames[DataType_max] =
{
	"float", "vec2", "vec3", "vec4",
	"mat3", "mat4",
	"int", "ivec2", "ivec3", "ivec4"
};

const std::string dataFormatNames[DataFormat_max] =
{
	"char",
	"unsigned char",
	"short",
	"unsigned short",
	"int",
	"insigned int",
	"half",
	"float",
	"double",
	"unsigned short <4444>",
	"unsigned short <5551>",
	"unsigned short <565>",
};

const std::map<IndexArrayFormat, std::pair<std::string, DataFormat>> indexArrayFormats =
{
	{ IndexArrayFormat::Format_8bit, {"8 bit", DataFormat::UnsignedChar} },
	{ IndexArrayFormat::Format_16bit, {"16 bit", DataFormat::UnsignedShort} },
	{ IndexArrayFormat::Format_32bit, {"32 bit", DataFormat::UnsignedInt} },
};

const std::string primitiveTypeNames[PrimitiveType_max] =
{
	"points", "lines", "triangles", "triangle strips", "line strips", "line strips adjancency",
};

const uint32_t vertexAttributeUsageMasks[VertexAttributeUsage_max] =
{
	0x0001, 0x0002, 0x0004, 0x0008, 0x0010,
	0x0020, 0x0040, 0x0080, 0x0100,
	0x0200, 0x0400, 0x0800, 0x1000, 0x2000,
	0x0000
};

VertexAttributeUsage et::stringToVertexAttributeUsage(const std::string& s, bool& builtIn)
{
	VertexAttributeUsage result = VertexAttributeUsage::Unknown;
	
	if (s.empty())
		return result;
	
	for (uint32_t i = 0, e = VertexAttributeUsage_max; i < e; ++i)
	{
		if (s == vertexAttributeUsageNames[i])
		{
			result = static_cast<VertexAttributeUsage>(i);
			break;
		}
	}
	
	if (result == VertexAttributeUsage::Unknown)
	{
		log::warning("Unknown vertex attribute usage: %s", s.c_str());
	}
	else
	{
		builtIn = (s.find("gl_") == 0);
	}
	
	return result;
}

DataType et::stringToDataType(const std::string& s)
{
	for (uint32_t i = 0, e = DataType_max; i < e; ++i)
	{
		if (s == dataTypeNames[i])
			return static_cast<DataType>(i);
	}
	
	return DataType::Float;
}

DataFormat et::stringToDataFormat(const std::string& s)
{
	for (uint32_t i = 0, e = DataFormat_max; i < e; ++i)
	{
		if (s == dataFormatNames[i])
			return static_cast<DataFormat>(i);
	}
	
	return DataFormat::Char;
}

IndexArrayFormat et::stringToIndexArrayFormat(const std::string& s)
{
	for (auto& kv : indexArrayFormats)
	{
		if (kv.second.first == s)
			return kv.first;
	}

	ET_FAIL("Invalid index array format string provided");
	return IndexArrayFormat::Format_32bit;
}

PrimitiveType et::stringToPrimitiveType(const std::string& s)
{
	for (uint32_t i = 0, e = PrimitiveType_max; i < e; ++i)
	{
		if (s == primitiveTypeNames[i])
			return static_cast<PrimitiveType>(i);
	}
	
	return PrimitiveType::Points;
}


std::string et::vertexAttributeUsageToString(VertexAttributeUsage va)
{
	return (va < VertexAttributeUsage::max) ? vertexAttributeUsageNames[static_cast<uint32_t>(va)] :
	intToStr(static_cast<uint32_t>(va));
}

std::string et::dataTypeToString(DataType vat)
{
	return (vat < DataType::max) ? dataTypeNames[static_cast<uint32_t>(vat)] :
	intToStr(static_cast<uint32_t>(vat));
}

std::string et::dataFormatToString(DataFormat dt)
{
	return (dt < DataFormat::max) ? dataFormatNames[static_cast<uint32_t>(dt)] :
	intToStr(static_cast<uint32_t>(dt));
}

std::string et::indexArrayFormatToString(IndexArrayFormat fmt)
{
	auto i = indexArrayFormats.find(fmt);
	ET_ASSERT(i != indexArrayFormats.end());
	return i->second.first;
}

et::DataFormat et::indexArrayFormatToDataFormat(IndexArrayFormat fmt)
{
	auto i = indexArrayFormats.find(fmt);
	ET_ASSERT(i != indexArrayFormats.end());
	return i->second.second;
}

std::string et::primitiveTypeToString(PrimitiveType pt)
{
	return (pt < PrimitiveType::max) ? primitiveTypeNames[static_cast<uint32_t>(pt)] :
	intToStr(static_cast<uint32_t>(pt));
}

uint32_t et::dataTypeComponents(DataType t)
{
	ET_ASSERT(t < DataType::max);
	
	static const uint32_t values[DataType_max] =
	{
		1,  // Float,
		2,  // Vec2,
		3,  // Vec3,
		4,  // Vec4,
		9,  // Mat3,
		16, // Mat4,
		1,  // Int,
		2,  // IntVec2,
		3,  // IntVec3,
		4,  // IntVec4,
	};
	return values[static_cast<uint32_t>(t)];
}

DataFormat et::dataTypeDataFormat(DataType t)
{
	ET_ASSERT(t < DataType::max);
	static const DataFormat values[DataType_max] =
	{
		DataFormat::Float, // Float,
		DataFormat::Float, // Vec2,
		DataFormat::Float, // Vec3,
		DataFormat::Float, // Vec4,
		DataFormat::Float, // Mat3,
		DataFormat::Float, // Mat4,
		DataFormat::Int, // Int,
		DataFormat::Int, // IntVec2,
		DataFormat::Int, // IntVec3,
		DataFormat::Int, // IntVec4,
	};
	return values[int32_t(t)];
}

uint32_t et::dataTypeSize(DataType t)
{
	return dataTypeComponents(t) * ((t == DataType::Int) ? sizeof(int) : sizeof(float));
}

uint32_t et::vertexAttributeUsageMask(VertexAttributeUsage u)
{
	ET_ASSERT(u < VertexAttributeUsage::max);
	return vertexAttributeUsageMasks[static_cast<uint32_t>(u)];
}

uint32_t et::sizeOfDataFormat(DataFormat type)
{
	switch (type)
	{
		case DataFormat::Char:
		case DataFormat::UnsignedChar:
			return 1;
			
		case DataFormat::Half:
		case DataFormat::Short:
		case DataFormat::UnsignedShort:
		case DataFormat::UnsignedShort_4444:
		case DataFormat::UnsignedShort_5551:
		case DataFormat::UnsignedShort_565:
			return 2;
			
		case DataFormat::Int:
		case DataFormat::UnsignedInt:
		case DataFormat::Float:
		case DataFormat::UnsignedInt_8888_Rev:
			return 4;
			
		case DataFormat::Double:
			return 8;
			
		default:
			ET_FAIL_FMT("Invalid data type: %d", type);
	}
}

uint32_t et::bitsPerPixelForTextureFormat(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::R8:
			return 8;
			
		case TextureFormat::R16:
		case TextureFormat::R16F:
		case TextureFormat::RG8:
		case TextureFormat::RGB565:
		case TextureFormat::Depth16:
			return 16;
			
		case TextureFormat::Depth24:
			return 24;
			
		case TextureFormat::RG16:
		case TextureFormat::RG16F:
		case TextureFormat::RGBA8:
		case TextureFormat::BGRA8:
		case TextureFormat::Depth32:
		case TextureFormat::Depth32F:
		case TextureFormat::R11G11B10F:
			return 32;
						
		case TextureFormat::RG32F:
		case TextureFormat::RGBA16:
		case TextureFormat::RGBA16F:
			return 64;
						
		case TextureFormat::RGBA32F:
			return 128;

		default:
		{
			ET_FAIL_FMT("Not yet implemented for this format: %u", static_cast<uint32_t>(format));
			return 0;
		}
	}
}

uint32_t et::bitsPerPixelForDataFormat(DataFormat type)
{
	switch (type)
	{
		case DataFormat::Char:
		case DataFormat::UnsignedChar:
			return 8;
			
		case DataFormat::Short:
		case DataFormat::UnsignedShort:
		case DataFormat::UnsignedShort_4444:
		case DataFormat::UnsignedShort_5551:
		case DataFormat::UnsignedShort_565:
			return 16;
			
		case DataFormat::Int:
		case DataFormat::UnsignedInt:
		case DataFormat::UnsignedInt_8888_Rev:
			return 32;
			
		case DataFormat::Half:
			return 16;
			
		case DataFormat::Float:
			return 32;
			
		case DataFormat::Double:
			return 64;
			
		default:
		{
			ET_FAIL_FMT("Unknown data type: %u", static_cast<uint32_t>(type));
			return 0;
		}
	}
}

uint32_t et::channelsForTextureFormat(TextureFormat internalFormat)
{
	switch (internalFormat)
	{
		case TextureFormat::Depth16:
		case TextureFormat::Depth24:
		case TextureFormat::Depth32:
		case TextureFormat::Depth32F:
		case TextureFormat::R8:
		case TextureFormat::R16:
		case TextureFormat::R16F:
		case TextureFormat::R32F:
		case TextureFormat::R11G11B10F:
			return 1;
			
		case TextureFormat::RG8:
		case TextureFormat::RG16:
		case TextureFormat::RG16F:
		case TextureFormat::RG32F:
		case TextureFormat::RGTC2:
			return 2;
			
		case TextureFormat::DXT1_RGB:
			return 3;
			
		case TextureFormat::RGBA8:
		case TextureFormat::BGRA8:
		case TextureFormat::RGBA16:
		case TextureFormat::RGBA16F:
		case TextureFormat::RGBA32F:
		case TextureFormat::DXT1_RGBA:
		case TextureFormat::DXT3:
		case TextureFormat::DXT5:
			return 4;
			
		default:
		{
			ET_FAIL_FMT("Channels not defined for format %u", static_cast<uint32_t>(internalFormat));
			return 0;
		}
	}
}

et::Dictionary et::serializeDepthState(const DepthState& _depth)
{
	Dictionary depth;
	depth.setBooleanForKey(kDepthWriteEnabled, _depth.depthWriteEnabled);
	depth.setStringForKey(kDepthFunction, compareFunctionToString(_depth.compareFunction));
	return depth;
}

et::Dictionary et::serializeBlendState(const BlendState& _blend)
{
	Dictionary blend;
	blend.setBooleanForKey(kBlendEnabled, _blend.enabled);
	
	BlendConfiguration config = BlendConfiguration::Disabled;
	if (blendStateToConfiguration(_blend, config))
	{
		blend.setStringForKey(kBlendConfiguration, blendConfigurationToString(config));
	}
	else
	{
		blend.setStringForKey(kSourceColor, blendFunctionToString(_blend.color.source));
		blend.setStringForKey(kDestColor, blendFunctionToString(_blend.color.dest));
		blend.setStringForKey(kSourceAlpha, blendFunctionToString(_blend.alpha.source));
		blend.setStringForKey(kDestAlpha, blendFunctionToString(_blend.alpha.dest));
		blend.setStringForKey(kColorOperation, blendOperationToString(_blend.colorOperation));
		blend.setStringForKey(kAlphaOperation, blendOperationToString(_blend.alphaOperation));
	}
	return blend;
}

DepthState et::deserializeDepthState(const et::Dictionary& depth)
{
	DepthState _depth;
	_depth.compareFunction = stringToCompareFunction(depth.stringForKey(kDepthFunction, compareFunctionToString(CompareFunction::Less))->content);
	_depth.depthWriteEnabled = depth.boolForKey(kDepthWriteEnabled, true)->content != 0;
	return _depth;
}

BlendState et::deserializeBlendState(const et::Dictionary& blend)
{
	BlendState _blend;
	
	BlendConfiguration config = BlendConfiguration::Disabled;
	if (stringToBlendConfiguration(blend.stringForKey(kBlendConfiguration)->content, config))
	{
		_blend = blendConfigurationToBlendState(config);
	}
	else
	{
		_blend.color.source = stringToBlendFunction(blend.stringForKey(kSourceColor, blendFunctionToString(BlendFunction::One))->content);
		_blend.color.dest = stringToBlendFunction(blend.stringForKey(kDestColor, blendFunctionToString(BlendFunction::Zero))->content);
		_blend.colorOperation = stringToBlendOperation(blend.stringForKey(kColorOperation, blendOperationToString(BlendOperation::Add))->content);
		_blend.alpha.source = stringToBlendFunction(blend.stringForKey(kSourceAlpha, blendFunctionToString(BlendFunction::One))->content);
		_blend.alpha.dest = stringToBlendFunction(blend.stringForKey(kDestAlpha, blendFunctionToString(BlendFunction::Zero))->content);
		_blend.alphaOperation = stringToBlendOperation(blend.stringForKey(kAlphaOperation, blendOperationToString(BlendOperation::Add))->content);
	}
	
	if (blend.hasKey(kBlendEnabled))
	{
		_blend.enabled = blend.boolForKey(kBlendEnabled)->content != 0;
	}
	return _blend;
}

namespace
{
	using BlendStateNameMap = std::pair<BlendState, std::string>;
	const BlendStateNameMap blendConfigToStateMap[BlendConfiguration_max] =
	{
		{ BlendState(0, BlendFunction::One, BlendFunction::Zero), "disabled"}, // Disabled,
		{ BlendState(1, BlendFunction::SourceAlpha, BlendFunction::InvSourceAlpha), "alpha-blend"}, // Default,
		{ BlendState(1, BlendFunction::One, BlendFunction::InvSourceAlpha), "alpha-premultiplied"}, // AlphaPremultiplied,
		{ BlendState(1, BlendFunction::One, BlendFunction::One), "additive"}, // Additive,
		{ BlendState(1, BlendFunction::SourceAlpha, BlendFunction::One), "alpha-additive"}, // AlphaAdditive,
		{ BlendState(1, BlendFunction::Zero, BlendFunction::SourceAlpha), "alpha-multiplicative"}, // AlphaMultiplicative,
		{ BlendState(1, BlendFunction::SourceColor, BlendFunction::One), "color-additive"}, // ColorAdditive,
		{ BlendState(1, BlendFunction::Zero, BlendFunction::InvSourceAlpha), "alpha-inverse-multiplicative"} // AlphaInverseMultiplicative,
	};
	
	std::string cullModeMap[CullMode_max] =
	{
		"disabled",
		"back",
		"front",
	};
}

bool et::blendStateToConfiguration(const BlendState& state, BlendConfiguration& config)
{
	uint32_t e = sizeof(blendConfigToStateMap) / sizeof(blendConfigToStateMap[0]);
	for (uint32_t i = 0; i < e; ++i)
	{
		if (blendConfigToStateMap[i].first == state)
		{
			config = static_cast<BlendConfiguration>(i);
			return true;
		}
	}
	return false;
}

bool et::stringToBlendConfiguration(const std::string& name, BlendConfiguration& config)
{
	uint32_t e = sizeof(blendConfigToStateMap) / sizeof(blendConfigToStateMap[0]);
	for (uint32_t i = 0; i < e; ++i)
	{
		if (blendConfigToStateMap[i].second == name)
		{
			config = static_cast<BlendConfiguration>(i);
			return true;
		}
	}
	return false;
}

BlendState et::blendConfigurationToBlendState(BlendConfiguration config)
{
	ET_ASSERT(config < BlendConfiguration::max);
	return blendConfigToStateMap[static_cast<uint32_t>(config)].first;
}

std::string et::blendConfigurationToString(BlendConfiguration config)
{
	ET_ASSERT(config < BlendConfiguration::max);
	return blendConfigToStateMap[static_cast<uint32_t>(config)].second;
}

std::string et::cullModeToString(CullMode mode)
{
	ET_ASSERT(mode < CullMode::max);
	return cullModeMap[static_cast<uint32_t>(mode)];
}

bool et::stringToCullMode(const std::string& mode, CullMode& outMode)
{
	uint32_t e = sizeof(cullModeMap) / sizeof(cullModeMap[0]);
	for (uint32_t i = 0; i < e; ++i)
	{
		if (mode == cullModeMap[i])
		{
			outMode = static_cast<CullMode>(i);
			return true;
		}
	}
	log::error("Unable to convert cull mode string `%s` to CullMode value", mode.c_str());
	return false;
}

template <class T>
using ValueNamePair = const std::pair<T, std::string>;

static ValueNamePair<et::CompareFunction> compareFunctionsMap[CompareFunction_max] =
{
	{et::CompareFunction::Never, "never"},
	{et::CompareFunction::Less, "less"},
	{et::CompareFunction::LessOrEqual, "less-or-equal"},
	{et::CompareFunction::Equal, "equal"},
	{et::CompareFunction::GreaterOrEqual, "greater-or-equal"},
	{et::CompareFunction::Greater, "greater"},
	{et::CompareFunction::Always, "always"},
};

static ValueNamePair<et::BlendFunction> blendFunctionsMap[BlendFunction_max] =
{
	{et::BlendFunction::Zero, "zero"}, // Zero,
	{et::BlendFunction::One, "one"}, // One,
	{et::BlendFunction::SourceColor, "src-color"}, // SourceColor,
	{et::BlendFunction::InvSourceColor, "inv-src-color"}, // InvSourceColor,
	{et::BlendFunction::SourceAlpha, "src-alpha"}, // SourceAlpha,
	{et::BlendFunction::InvSourceAlpha, "inv-src-alpha"}, // InvSourceAlpha,
	{et::BlendFunction::DestColor, "dst-color"}, // DestColor,
	{et::BlendFunction::InvDestColor, "inv-dst-color"}, // InvDestColor,
	{et::BlendFunction::DestAlpha, "dst-alpha"}, // DestAlpha,
	{et::BlendFunction::InvDestAlpha, "inv-dst-alpha"}, // InvDestAlpha,
};

static ValueNamePair<et::BlendOperation> blendOperationsMap[BlendOperation_max] =
{
	{et::BlendOperation::Add, "add"}, // Add,
	{et::BlendOperation::Subtract, "subtract"}, // Subtract,
	{et::BlendOperation::ReverseSubtract, "reverse-subtract"}, // ReverseSubtract,
};

template <class ENUM>
const ValueNamePair<ENUM>& sampleValueFromMap(ENUM value, const ValueNamePair<ENUM>* fromMap)
{
	ET_ASSERT(value < ENUM::max);
	return fromMap[static_cast<uint32_t>(value)];
}

template <class ENUM>
ENUM findValueInMap(uint32_t value, const ValueNamePair<ENUM>* inMap, size_t mapSize)
{
	for (size_t i = 0; i < mapSize; ++i)
	{
		if (value == inMap[i].first)
			return static_cast<ENUM>(i);
	}
	log::error("Unable to find enum value in map: %x (%s - %s)",
		value, inMap[0].second.c_str(), inMap[mapSize-1].second.c_str());
	return static_cast<ENUM>(0);
}

template <class ENUM>
ENUM findStringInMap(const std::string& value, const ValueNamePair<ENUM>* inMap, size_t mapSize)
{
	for (size_t i = 0; i < mapSize; ++i)
	{
		if (value == inMap[i].second)
			return static_cast<ENUM>(i);
	}
	log::error("Unable to find enum value in map: %s (%s - %s)",
		value.c_str(), inMap[0].second.c_str(), inMap[mapSize-1].second.c_str());
	return static_cast<ENUM>(0);
}

const std::string& et::compareFunctionToString(CompareFunction value)
{
	return sampleValueFromMap(value, compareFunctionsMap).second;
}

const std::string& et::blendFunctionToString(BlendFunction value)
{
	return sampleValueFromMap(value, blendFunctionsMap).second;
}

const std::string& et::blendOperationToString(BlendOperation value)
{
	return sampleValueFromMap(value, blendOperationsMap).second;
}

CompareFunction et::stringToCompareFunction(const std::string& value)
{
	return findStringInMap<CompareFunction>(value, compareFunctionsMap, sizeof(compareFunctionsMap) / sizeof(compareFunctionsMap[0]));
}

BlendFunction et::stringToBlendFunction(const std::string& value)
{
	return findStringInMap<BlendFunction>(value, blendFunctionsMap, sizeof(blendFunctionsMap) / sizeof(blendFunctionsMap[0]));
}

BlendOperation et::stringToBlendOperation(const std::string& value)
{
	return findStringInMap<BlendOperation>(value, blendOperationsMap, sizeof(blendOperationsMap) / sizeof(blendOperationsMap[0]));
}