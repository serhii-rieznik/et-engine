/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/variableset.h>

namespace et
{

static const std::map<ObjectVariable, std::string> objectVariableNames =
{
	{ ObjectVariable::WorldTransform, "worldTransform" },
	{ ObjectVariable::WorldRotationTransform, "worldRotationTransform" },

	{ ObjectVariable::ViewTransform, "viewTransform" },
	{ ObjectVariable::ProjectionTransform, "projectionTransform" },
	{ ObjectVariable::ViewProjectionTransform, "viewProjectionTransform" },
	{ ObjectVariable::InverseViewTransform, "inverseViewTransform" },
	{ ObjectVariable::InverseProjectionTransform, "inverseProjectionTransform" },
	{ ObjectVariable::InverseViewProjectionTransform, "inverseViewProjectionTransform" },

	{ ObjectVariable::PreviousWorldTransform, "previousWorldTransform" },
	{ ObjectVariable::PreviousWorldRotationTransform, "previousWorldRotationTransform" },

	{ ObjectVariable::PreviousViewTransform, "previousViewTransform" },
	{ ObjectVariable::PreviousProjectionTransform, "previousProjectionTransform" },
	{ ObjectVariable::PreviousViewProjectionTransform, "previousViewProjectionTransform" },
	{ ObjectVariable::PreviousInverseViewTransform, "previousInverseViewTransform" },
	{ ObjectVariable::PreviousInverseProjectionTransform, "previousInverseProjectionTransform" },
	{ ObjectVariable::PreviousInverseViewProjectionTransform, "previousInverseViewProjectionTransform" },

	{ ObjectVariable::CameraPosition, "cameraPosition" },
	{ ObjectVariable::CameraDirection, "cameraDirection" },
	{ ObjectVariable::CameraClipPlanes, "cameraClipPlanes" },
	{ ObjectVariable::CameraJitter, "cameraJitter" },

	{ ObjectVariable::LightColor, "lightColor" },
	{ ObjectVariable::LightDirection, "lightDirection" },
	{ ObjectVariable::LightViewTransform, "lightViewTransform" },
	{ ObjectVariable::LightProjectionTransform, "lightProjectionTransform" },
	{ ObjectVariable::ContinuousTime, "continuousTime" },
	{ ObjectVariable::DeltaTime, "deltaTime" },
	{ ObjectVariable::Viewport, "viewport" },
};

static const std::map<MaterialVariable, std::string> materialVariableNames =
{
	{ MaterialVariable::DiffuseReflectance, "diffuseReflectance" },
	{ MaterialVariable::SpecularReflectance, "specularReflectance" },
	{ MaterialVariable::NormalScale, "normalScale" },
	{ MaterialVariable::RoughnessScale, "roughnessScale" },
	{ MaterialVariable::MetallnessScale, "metallnessScale" },
	{ MaterialVariable::EmissiveColor, "emissiveColor" },
	{ MaterialVariable::OpacityScale, "opacityScale" },
	{ MaterialVariable::IndexOfRefraction, "indexOfRefraction" },
	{ MaterialVariable::SpecularExponent, "specularExponent" },
	{ MaterialVariable::ExtraParameters, "extraParameters" },
};

const std::string& objectVariableToString(ObjectVariable p)
{
	ET_ASSERT(p < ObjectVariable::max);
	return objectVariableNames.at(p);
}

const std::string& materialVariableToString(MaterialVariable p)
{
	ET_ASSERT(p < MaterialVariable::max);
	return materialVariableNames.at(p);
}

const std::map<MaterialTexture, std::string>& materialTextureNames()
{
	static const std::map<MaterialTexture, std::string> localMap =
	{
		{ MaterialTexture::BaseColor, "baseColorTexture" },
		{ MaterialTexture::Normal, "normalTexture" },
		{ MaterialTexture::Roughness, "roughnessTexture" },
		{ MaterialTexture::Metallness, "metallnessTexture" },
		{ MaterialTexture::EmissiveColor, "emissiveColorTexture" },
		{ MaterialTexture::Opacity, "opacityTexture" },
		{ MaterialTexture::Shadow, "shadowTexture" },
		{ MaterialTexture::AmbientOcclusion, "aoTexture" },
		{ MaterialTexture::ConvolvedDiffuse, "convolvedDiffuseTexture" },
		{ MaterialTexture::ConvolvedSpecular, "convolvedSpecularTexture" },
		{ MaterialTexture::BRDFLookup, "brdfLookupTexture" },
		{ MaterialTexture::Noise, "NoiseTexture" },
	};
	return localMap;
}

const std::map<MaterialTexture, std::string>& materialSamplerNames()
{
	static const std::map<MaterialTexture, std::string> names =
	{
		{ MaterialTexture::BaseColor, "baseColorSampler" },
		{ MaterialTexture::Normal, "normalSampler" },
		{ MaterialTexture::Roughness, "roughnessSampler" },
		{ MaterialTexture::Metallness, "metallnessSampler" },
		{ MaterialTexture::EmissiveColor, "emissiveColorSampler" },
		{ MaterialTexture::Opacity, "opacitySampler" },
		{ MaterialTexture::Shadow, "shadowSampler" },
		{ MaterialTexture::AmbientOcclusion, "aoSampler" },
		{ MaterialTexture::ConvolvedDiffuse, "convolvedDiffuseSampler" },
		{ MaterialTexture::ConvolvedSpecular, "convolvedSpecularSampler" },
		{ MaterialTexture::BRDFLookup, "brdfLookupSampler" },
		{ MaterialTexture::Noise, "NoiseSampler" },
	};
	return names;
}

static const std::map<StorageBuffer, std::string>& storageBufferNames()
{
	static const std::map<StorageBuffer, std::string> localMap =
	{
		{ StorageBuffer::StorageBuffer0, "storageBuffer0" },
		{ StorageBuffer::StorageBuffer1, "storageBuffer1" },
		{ StorageBuffer::StorageBuffer2, "storageBuffer2" },
		{ StorageBuffer::StorageBuffer3, "storageBuffer3" },
	};
	return localMap;
}

const std::string& materialTextureToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::max);
	return materialTextureNames().at(t);
}

MaterialTexture stringToMaterialTexture(const std::string& name)
{
	for (const auto& ts : materialTextureNames())
	{
		if (ts.second == name)
			return ts.first;
	}
	return MaterialTexture::max;
}

const std::string& materialSamplerToString(MaterialTexture t)
{
	ET_ASSERT(t < MaterialTexture::max);
	return materialSamplerNames().at(t);
}

MaterialTexture samplerToMaterialTexture(const std::string& name)
{
	for (const auto& ts : materialSamplerNames())
	{
		if (ts.second == name)
			return ts.first;
	}
	return MaterialTexture::max;
}

ObjectVariable stringToObjectVariable(const std::string& name)
{
	for (const auto& ts : objectVariableNames)
	{
		if (ts.second == name)
			return static_cast<ObjectVariable>(ts.first);
	}
	return ObjectVariable::max;
}

MaterialVariable stringToMaterialVariable(const std::string& name)
{
	for (const auto& ts : materialVariableNames)
	{
		if (ts.second == name)
			return static_cast<MaterialVariable>(ts.first);
	}
	return MaterialVariable::max;
}

const std::string& storageBufferToString(StorageBuffer t)
{
	ET_ASSERT(t < StorageBuffer::max);
	return storageBufferNames().at(t);
}

StorageBuffer stringToStorageBuffer(const std::string& name)
{
	for (const auto& ts : storageBufferNames())
	{
		if (ts.second == name)
			return static_cast<StorageBuffer>(ts.first);
	}
	return StorageBuffer::max;
}

}
