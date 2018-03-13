/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/variableset.h>

namespace et {

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

	{ ObjectVariable::EnvironmentSphericalHarmonics, "environmentSphericalHarmonics" },
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

const std::string& objectVariableToString(ObjectVariable p) {
	ET_ASSERT(p < ObjectVariable::max);
	return objectVariableNames.at(p);
}

const std::string& materialVariableToString(MaterialVariable p) {
	ET_ASSERT(p < MaterialVariable::max);
	return materialVariableNames.at(p);
}

namespace MaterialTexture {

const std::string MaterialTexture::BaseColor = "baseColor";
const std::string MaterialTexture::Normal = "normal";
const std::string MaterialTexture::Opacity = "opacity";
const std::string MaterialTexture::EmissiveColor = "emissiveColor";
const std::string MaterialTexture::Shadow = "shadow";
const std::string MaterialTexture::AmbientOcclusion = "ao";
const std::string MaterialTexture::ConvolvedSpecular = "convolvedSpecular";
const std::string MaterialTexture::BRDFLookup = "brdfLookup";
const std::string MaterialTexture::Noise = "noise";
const std::string MaterialTexture::LTCTransform = "ltcTransform";
const std::string MaterialTexture::Input = "inputTexture";
const std::string MaterialTexture::OutputImage = "outputImage";

}

ObjectVariable stringToObjectVariable(const std::string& name) {
	for (const auto& ts : objectVariableNames)
	{
		if (ts.second == name)
			return static_cast<ObjectVariable>(ts.first);
	}
	return ObjectVariable::max;
}

MaterialVariable stringToMaterialVariable(const std::string& name) {
	for (const auto& ts : materialVariableNames)
	{
		if (ts.second == name)
			return static_cast<MaterialVariable>(ts.first);
	}
	return MaterialVariable::max;
}

}
