/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>
#include <et/rendering/interface/sampler.h>

namespace et
{
enum class ObjectVariable : uint32_t
{
	WorldTransform,
	WorldRotationTransform,
	PreviousWorldTransform,
	PreviousWorldRotationTransform,

	ViewTransform,
	InverseViewTransform,
	ProjectionTransform,
	InverseProjectionTransform,
	ViewProjectionTransform,
	InverseViewProjectionTransform,

	PreviousViewTransform,
	PreviousInverseViewTransform,
	PreviousProjectionTransform,
	PreviousInverseProjectionTransform,
	PreviousViewProjectionTransform,
	PreviousInverseViewProjectionTransform,

	CameraPosition,
	CameraDirection,
	CameraClipPlanes,
	CameraJitter,

	LightColor,
	LightDirection,
	LightViewTransform,
	LightProjectionTransform,

	DeltaTime,

	max
};

enum class MaterialVariable : uint32_t
{
	DiffuseReflectance,
	SpecularReflectance,
	EmissiveColor,
	NormalScale,
	RoughnessScale,
	MetallnessScale,
	OpacityScale,
	IndexOfRefraction,
	SpecularExponent,
	ExtraParameters,

	max
};

enum class MaterialTexture : uint32_t
{
	// per-object textures
	BaseColor,
	Normal,
	Roughness,
	Metallness,
	EmissiveColor,
	Opacity,

	Shadow,
	AmbientOcclusion,
	Environment,
	BRDFLookup,

	max,

	// service values
	FirstMaterialTexture = BaseColor,
	LastMaterialTexture = EmissiveColor,

	FirstSharedTexture = Shadow,
	LastSharedTexture = BRDFLookup,
};

enum class StorageBuffer
{
	StorageBuffer0,
	StorageBuffer1,
	StorageBuffer2,
	StorageBuffer3,

	max
};

enum : uint32_t
{
	MaterialSamplerBindingOffset = 16,

	ObjectVariable_max = static_cast<uint32_t>(ObjectVariable::max),
	MaterialVariable_max = static_cast<uint32_t>(MaterialVariable::max),
	MaterialTexture_max = static_cast<uint32_t>(MaterialTexture::max),
	
	StorageBuffer_max = static_cast<uint32_t>(StorageBuffer::max),
};

struct MaterialTextureHolder
{
	MaterialTexture binding = MaterialTexture::max;
	Texture::Pointer texture;
	uint32_t index = 0;
};
using MaterialTexturesCollection = UnorderedMap<String, MaterialTextureHolder>;

struct MaterialSamplerHolder
{
	MaterialTexture binding = MaterialTexture::max;
	Sampler::Pointer sampler;
	uint32_t index = 0;
};
using MaterialSamplersCollection = UnorderedMap<String, MaterialSamplerHolder>;

struct MaterialPropertyHolder
{
	MaterialVariable binding = MaterialVariable::max;
	char data[sizeof(mat4)] { };
	uint32_t size = 0;
};
using MaterialPropertiesCollection = UnorderedMap<String, MaterialPropertyHolder>;

template <class T>
struct OptionalTextureObject
{
	T object;
	MaterialTexture binding = MaterialTexture::max;
	uint32_t index = 0;

	void clear()
	{
		index = 0;
		object = nullptr;
	}
};
template <class T>
struct OptionalImageObject
{
	T object;
	StorageBuffer binding = StorageBuffer::max;
	uint32_t index = 0;

	void clear()
	{
		index = 0;
		object = nullptr;
	}
};
using TexturesHolder = std::map<uint32_t, OptionalTextureObject<Texture::Pointer>>;
using SamplersHolder = std::map<uint32_t, OptionalTextureObject<Sampler::Pointer>>;
using ImagesHolder = std::map<uint32_t, OptionalImageObject<Texture::Pointer>>;

struct OptionalValue
{
	DataType storedType = DataType::max;
	uint32_t binding = InvalidIndex;
	char data[sizeof(mat4)] { };
	uint32_t size = 0;

	bool isSet() const
		{ return storedType != DataType::max; }

	template <class T>
	const T& as() const
	{
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		ET_ASSERT(is<T>());
		return *(reinterpret_cast<const T*>(data));
	};

	template <class T>
	bool is() const
	{
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		return storedType == dataTypeFromClass<T>();
	}

	template <class T>
	void operator = (const T& value)
	{
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		*(reinterpret_cast<T*>(data)) = value;
		storedType = dataTypeFromClass<T>();
		size = sizeof(value);
	}

	void clear()
	{
		memset(data, 0, sizeof(data));
		storedType = DataType::max;
	}
};
using VariablesHolder = std::map<uint32_t, OptionalValue>;

const std::string& objectVariableToString(ObjectVariable);
ObjectVariable stringToObjectVariable(const std::string&);

const std::string& materialVariableToString(MaterialVariable);
MaterialVariable stringToMaterialVariable(const std::string&);

const std::string& materialSamplerToString(MaterialTexture);
MaterialTexture samplerToMaterialTexture(const std::string&);

const std::string& materialTextureToString(MaterialTexture);
MaterialTexture stringToMaterialTexture(const std::string&);

const std::string& storageBufferToString(StorageBuffer);
StorageBuffer stringToStorageBuffer(const std::string&);

}
