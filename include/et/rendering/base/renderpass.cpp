/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/renderpass.h>
#include <et/rendering/interface/renderer.h>

namespace et {

const std::string RenderPass::kPassNameDefault = "default";
const std::string RenderPass::kPassNameDepth = "depth";
const std::string RenderPass::kPassNameUI = "ui";

RenderPass::ConstructionInfo RenderPass::renderTargetPassInfo(const std::string& name, const Texture::Pointer& texture) {
	ConstructionInfo result;
	result.name = name;
	result.color[0].texture = texture;
	result.color[0].loadOperation = FramebufferOperation::Clear;
	result.color[0].targetClass = RenderTarget::Class::Texture;
	result.color[0].storeOperation = FramebufferOperation::Store;
	return result;
}

RenderPass::RenderPass(RenderInterface* renderer, const ConstructionInfo& info) :
	_renderer(renderer), _info(info) {
}

const RenderPass::ConstructionInfo& RenderPass::info() const {
	return _info;
}

void RenderPass::setSharedTexture(const std::string& texId, const Texture::Pointer& tex) {
	_sharedTextures[texId].first = tex;
}

void RenderPass::setSharedTexture(const std::string& texId, const Texture::Pointer& tex, const Sampler::Pointer& smp) {
	setSharedTexture(texId, tex);
	_sharedTextures[texId].second = smp;
}

uint64_t RenderPass::identifier() const {
	return reinterpret_cast<uintptr_t>(this);
}

void RenderPass::loadSharedVariablesFromCamera(const Camera::Pointer& cam) {
	using matFunction = const mat4& (Camera::*)() const;
	using tpl = std::tuple<ObjectVariable, ObjectVariable, matFunction>;

	static const tpl variables[] =
	{
		{ ObjectVariable::ViewTransform, ObjectVariable::PreviousViewTransform, &Camera::viewMatrix },
		{ ObjectVariable::InverseViewTransform, ObjectVariable::PreviousInverseViewTransform, &Camera::inverseViewMatrix },
		{ ObjectVariable::ProjectionTransform, ObjectVariable::PreviousProjectionTransform, &Camera::projectionMatrix },
		{ ObjectVariable::InverseProjectionTransform, ObjectVariable::PreviousInverseProjectionTransform, &Camera::inverseProjectionMatrix },
		{ ObjectVariable::ViewProjectionTransform, ObjectVariable::PreviousViewProjectionTransform, &Camera::viewProjectionMatrix },
		{ ObjectVariable::InverseViewProjectionTransform, ObjectVariable::PreviousInverseViewProjectionTransform, &Camera::inverseViewProjectionMatrix },
	};

	const Camera* camera = cam.pointer();
	for (const auto& var : variables)
	{
		const mat4& actualValue = (camera->*(std::get<2>(var)))();

		mat4 previousValue = actualValue;
		loadSharedVariable(std::get<0>(var), previousValue);

		setSharedVariable(std::get<0>(var), actualValue);
		setSharedVariable(std::get<1>(var), previousValue);
	}

	setSharedVariable(ObjectVariable::CameraPosition, vec4(cam->position(), 1.0f));
	setSharedVariable(ObjectVariable::CameraDirection, vec4(cam->direction(), 0.0f));
	setSharedVariable(ObjectVariable::CameraClipPlanes, vec2(cam->zNear(), cam->zFar()));
}

void RenderPass::loadSharedVariablesFromLight(const Light::Pointer& l) {
	setSharedVariable(ObjectVariable::LightColor, vec4(l->color(), 1.0f));
	setSharedVariable(ObjectVariable::LightDirection, vec4(l->direction(), 0.0f));
	setSharedVariable(ObjectVariable::LightViewTransform, l->viewMatrix());
	setSharedVariable(ObjectVariable::LightProjectionTransform, l->projectionMatrix());
}

}
