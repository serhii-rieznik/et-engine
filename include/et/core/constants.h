/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

#define PI								3.1415926535897932384626433832795f
#define DOUBLE_PI						6.283185307179586476925286766559f
#define HALF_PI							1.5707963267948966192313216916398f
#define QUARTER_PI						0.78539816339744830961566084581988f

#define DEG_1							0.01745329251994329576923690768489f
#define DEG_15							0.26179938779914943653855361527329f
#define DEG_30							0.52359877559829887307710723054658f
#define DEG_45							0.78539816339744830961566084581988f
#define DEG_60							1.0471975511965977461542144610932f
#define DEG_90							1.5707963267948966192313216916398f

#define TO_DEGREES						57.295779513082320876798154814105f
#define TO_RADIANS						0.01745329251994329576923690768489f

#define SQRT_2							1.4142135623730950488016887242097f
#define INV_SQRT_2						0.70710678118654752440084436210485f

#define SQRT_3							1.732050807568877f
#define INV_SQRT_3						0.577350269189626f

#define LN_2							0.693147180559945f
#define INV_LN_2						1.44269504088896f

#define GOLDEN_RATIO					1.618033988749895f

#define HORSEPOWER_TO_WATT				735.49875f

#define ET_BACKSPACE					8
#define ET_TAB							9
#define ET_NEWLINE						10
#define ET_RETURN						13
#define ET_SPACE						32

#if (ET_PLATFORM_MAC)
#
#	define ET_KEY_RETURN				36
#	define ET_KEY_TAB					48
#	define ET_KEY_SPACE					49
#	define ET_KEY_ESCAPE				53
#	define ET_KEY_BACKSPACE				51
#	define ET_KEY_SHIFT					56
#	define ET_KEY_CONTROL				1000000
#	define ET_KEY_GRAY_PLUS				1000000
#	define ET_KEY_GRAY_MINUS			1000000
#
#	define ET_KEY_LEFT					123
#	define ET_KEY_RIGHT					124
#	define ET_KEY_DOWN					125
#	define ET_KEY_UP					126
#
#	define ET_KEY_W						13
#	define ET_KEY_A						0
#	define ET_KEY_S						1
#	define ET_KEY_D						2
#	define ET_KEY_H						4
#
#	define ET_KEY_1						10000000
#	define ET_KEY_2						10000000
#	define ET_KEY_3						10000000
#	define ET_KEY_4						10000000
#
#
#elif (ET_PLATFORM_WIN)
#
#	define ET_KEY_RETURN				0x0D
#	define ET_KEY_TAB					0x09
#	define ET_KEY_SPACE					0x20
#	define ET_KEY_ESCAPE				0x1B
#	define ET_KEY_BACKSPACE				0x08
#	define ET_KEY_SHIFT					0x10
#	define ET_KEY_CONTROL				0x11
#
#	define ET_KEY_LEFT					0x25
#	define ET_KEY_RIGHT					0x27
#	define ET_KEY_DOWN					0x28
#	define ET_KEY_UP					0x26
#
#	define ET_KEY_A						'A'	
#	define ET_KEY_S						'S'	
#	define ET_KEY_D						'D'	
#	define ET_KEY_W						'W'	
#	define ET_KEY_H						'H'	
#	define ET_KEY_1						49
#	define ET_KEY_2						50
#	define ET_KEY_3						51
#	define ET_KEY_4						52
#	define ET_KEY_GRAY_PLUS				107
#	define ET_KEY_GRAY_MINUS			109
#	define ET_KEY_GRAY_MULTIPLY			106
#	define ET_KEY_GRAY_DIVIDE			111
#
#endif

#define ET_DEFAULT_DELIMITER			';'
#define ET_DEFAULT_DELIMITER_STRING		";"

namespace et
{
extern const std::string kStorage;
extern const std::string kName;
extern const std::string kCompute;
extern const std::string kClass;
extern const std::string kChildren;
extern const std::string kElementTypeCode;
extern const std::string kRenderBatches;
extern const std::string kMaterialName;
extern const std::string kFlagsValue;
extern const std::string kTransform;
extern const std::string kAdditionalTransform;
extern const std::string kTranslation;
extern const std::string kScale;
extern const std::string kOrientation;
extern const std::string kProperties;
extern const std::string kAnimations;
extern const std::string kMaterials;
extern const std::string kBlendState;
extern const std::string kDepthState;
extern const std::string kIntegerValues;
extern const std::string kFloatValues;
extern const std::string kVectorValues;
extern const std::string kTextures;
extern const std::string kLods;
extern const std::string kStartIndex;
extern const std::string kIndexesCount;
extern const std::string kVertexArrayName;
extern const std::string kVertexStorageName;
extern const std::string kIndexArrayName;
extern const std::string kMinMaxCenter;
extern const std::string kAverageCenter;
extern const std::string kDimensions;
extern const std::string kBoundingSphereRadius;
extern const std::string kSupportData;
extern const std::string kVertexStorages;
extern const std::string kIndexArray;
extern const std::string kBinary;
extern const std::string kVertexDeclaration;
extern const std::string kUsage;
extern const std::string kType;
extern const std::string kDataType;
extern const std::string kStride;
extern const std::string kOffset;
extern const std::string kComponents;
extern const std::string kDataSize;
extern const std::string kFormat;
extern const std::string kPrimitiveType;
extern const std::string kView;
extern const std::string kProjection;
extern const std::string kUpVector;
extern const std::string kUpVectorLocked;
extern const std::string kVertexBufferName;
extern const std::string kIndexBufferName;
extern const std::string kVertexBufferSourceName;
extern const std::string kIndexBufferSourceName;
extern const std::string kVertexArrayObjects;
extern const std::string kVertexArrayObjectName;
extern const std::string kBlendEnabled;
extern const std::string kBlendConfiguration;
extern const std::string kSourceColor;
extern const std::string kDestColor;
extern const std::string kSourceAlpha;
extern const std::string kDestAlpha;
extern const std::string kColorOperation;
extern const std::string kAlphaOperation;
extern const std::string kDepthFunction;
extern const std::string kDepthWriteEnabled;
extern const std::string kDepthTestEnabled;
extern const std::string kDepthClearValue;
extern const std::string kCullMode;
}
