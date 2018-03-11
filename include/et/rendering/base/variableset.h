/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>
#include <et/rendering/interface/sampler.h>

namespace et {
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

	ContinuousTime,
	DeltaTime,
	Viewport,

	EnvironmentSphericalHarmonics,

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

namespace MaterialTexture {

extern const std::string BaseColor;
extern const std::string Normal;
extern const std::string EmissiveColor;
extern const std::string Opacity;
extern const std::string Shadow;
extern const std::string AmbientOcclusion;
extern const std::string ConvolvedSpecular;
extern const std::string BRDFLookup;
extern const std::string Noise;
extern const std::string LTCTransform;

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
	StorageBuffer_max = static_cast<uint32_t>(StorageBuffer::max),
};

struct MaterialTextureHolder
{
	std::string binding;
	Texture::Pointer texture;
	uint32_t index = 0;
};
using MaterialTexturesCollection = UnorderedMap<String, MaterialTextureHolder>;

struct MaterialSamplerHolder
{
	std::string binding;
	Sampler::Pointer sampler;
	uint32_t index = 0;
};
using MaterialSamplersCollection = UnorderedMap<String, MaterialSamplerHolder>;

struct MaterialPropertyHolder
{
	MaterialVariable binding = MaterialVariable::max;
	char data[sizeof(mat4)]{ };
	uint32_t size = 0;
};
using MaterialPropertiesCollection = UnorderedMap<String, MaterialPropertyHolder>;

template <class T>
struct OptionalObject
{
	IntrusivePtr<T> object;
	 
	void clear() {
		object.reset(nullptr);
	}
};

struct OptionalTextureObject : public OptionalObject<Texture>
{
	ResourceRange range;
	std::string binding;
};

struct OptionalSamplerObject : public OptionalObject<Sampler>
{
	std::string binding;
};

struct OptionalImageObject : public OptionalObject<Texture>
{
	StorageBuffer binding = StorageBuffer::max;
};

using TexturesHolder = UnorderedMap<std::string, OptionalTextureObject>;
using SamplersHolder = UnorderedMap<std::string, OptionalSamplerObject>;
using ImagesHolder = std::map<uint32_t, OptionalImageObject>;

struct OptionalValue
{
	DataType storedType = DataType::max;
	uint32_t binding = InvalidIndex;
	char data[256]{ };
	uint32_t dataSize = 0;
	uint32_t elementCount = 0;

	bool isSet() const {
		return storedType != DataType::max;
	}

	template <class T>
	const T& as() const {
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		ET_ASSERT(is<T>());
		return *(reinterpret_cast<const T*>(data));
	};

	template <class T>
	const T* asPointer() const {
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		ET_ASSERT(is<T>());
		return (reinterpret_cast<const T*>(data));
	}

	template <class T>
	bool is() const {
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		return storedType == dataTypeFromClass<T>();
	}

	template <class T>
	void set(const T* value, uint32_t count) {
		static_assert(sizeof(T) <= sizeof(data), "Requested type is too large");
		elementCount = count;
		dataSize = sizeof(T) * elementCount;
		ET_ASSERT(dataSize <= sizeof(data));
		memcpy(data, value, dataSize);
		storedType = dataTypeFromClass<T>();
	}

	template <class T>
	void set(const T& value) {
		set(&value, 1);
	}

	void clear() {
		memset(data, 0, sizeof(data));
		storedType = DataType::max;
		elementCount = 0;
		dataSize = 0;
	}
};
using VariablesHolder = Map<uint32_t, OptionalValue>;

const std::string& objectVariableToString(ObjectVariable);
ObjectVariable stringToObjectVariable(const std::string&);

const std::string& materialVariableToString(MaterialVariable);
MaterialVariable stringToMaterialVariable(const std::string&);

const std::string& storageBufferToString(StorageBuffer);
StorageBuffer stringToStorageBuffer(const std::string&);

}
