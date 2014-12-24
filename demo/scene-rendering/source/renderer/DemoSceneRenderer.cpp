//
//  DemoSceneRenderer.cpp
//  SceneRendering
//
//  Created by Sergey Reznik on 14/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/primitives/primitives.h>
#include "DemoSceneRenderer.h"

using namespace et;
using namespace demo;

#define ENABLE_DEBUG_RENDERING	0

extern const std::string basicVertexShader;
extern const std::string basicFragmentShader;

const float baseFrameTime = 1.0f / 30.0f;

enum
{
	diffuseTextureUnit = 0,
	normalTextureUnit = 1,
	transparencyTextureUnit = 2,
	depthTextureUnit = 3,
	noiseTextureUnit = 4,
	occlusionTextureUnit = 5,
};

void SceneRenderer::init(et::RenderContext* rc)
{
	_rc = rc;
	
	_finalBuffers[0] = rc->framebufferFactory().createFramebuffer(rc->sizei(), "final-buffer-1",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
	_finalBuffers[1] = rc->framebufferFactory().createFramebuffer(rc->sizei(), "final-buffer-2",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
	_geometryBuffer = rc->framebufferFactory().createFramebuffer(rc->sizei(), "geometry-buffer");
	
	_geometryBuffer->addSameRendertarget();
	
	_downsampledBuffer = rc->framebufferFactory().createFramebuffer(rc->sizei() / 2, "downsampled-buffer",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
	_downsampledBuffer->addSameRendertarget();
	
	_defaultTexture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, BinaryDataStorage(4, 255), "white-texture");
	
	_noiseTexture = _rc->textureFactory().genNoiseTexture(vec2i(256), true, "noise-texture");
	
	for (size_t i = 0; i < 10; ++i)
	{
		float t = (static_cast<float>(i) / 9.0f) * 1500.0f - 750.0f;
		_lightPositions.push_back(vec3(t, randomFloat(5.0f, 200.0f), 0.0f));
	}
	
	BinaryDataStorage normalData(4, 128);
	normalData[2] = 255;
	
	_defaultNormalTexture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, normalData, "normal-texture");
	
	ObjectsCache localCache;
	programs.prepass = _rc->programFactory().loadProgram("data/shaders/prepass.program", localCache);
	programs.prepass->setUniform("texture_mask", transparencyTextureUnit);
	programs.prepass->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.prepass->setUniform("texture_normal", normalTextureUnit);

	programs.fxaa = _rc->programFactory().loadProgram("data/shaders/fxaa.program", localCache);
	programs.fxaa->setUniform("texture_color", diffuseTextureUnit);

	programs.motionBlur = _rc->programFactory().loadProgram("data/shaders/motionblur.program", localCache);
	programs.motionBlur->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.motionBlur->setUniform("texture_depth", depthTextureUnit);
	
	programs.ambientOcclusion = _rc->programFactory().loadProgram("data/shaders/ao.program", localCache);
	programs.ambientOcclusion->setUniform("texture_depth", depthTextureUnit);
	programs.ambientOcclusion->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.ambientOcclusion->setUniform("texture_normal", normalTextureUnit);
	programs.ambientOcclusion->setUniform("texture_noise", noiseTextureUnit);
	programs.ambientOcclusion->setUniform("noiseTextureScale", vector2ToFloat(_downsampledBuffer->size()) / _noiseTexture->sizeFloat());
	programs.ambientOcclusion->setUniform("texel", vec2(1.0f) / vector2ToFloat(_geometryBuffer->size()));
	
	programs.ambientOcclusionBlur = _rc->programFactory().loadProgram("data/shaders/blur.program", localCache);
	programs.ambientOcclusionBlur->setUniform("texture_depth", depthTextureUnit);
	programs.ambientOcclusionBlur->setUniform("texture_color", diffuseTextureUnit);
	programs.ambientOcclusionBlur->setUniform("texture_normal", normalTextureUnit);
	
	programs.final = _rc->programFactory().loadProgram("data/shaders/final.program", localCache);
	programs.final->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.final->setUniform("texture_normal", normalTextureUnit);
	programs.final->setUniform("texture_depth", depthTextureUnit);
	programs.final->setUniform("texture_occlusion", occlusionTextureUnit);
	
	{
		VertexDeclaration decl(true, Usage_Position, Type_Vec3);
		decl.push_back(Usage_TexCoord0, Type_Vec3);
		decl.push_back(Usage_Normal, Type_Vec3);
		decl.push_back(Usage_Tangent, Type_Vec3);
		
		VertexArray::Pointer va = VertexArray::Pointer::create(decl, 0);
		primitives::createCube(va, 10.0f);
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat_16bit, va->size(), PrimitiveType_Triangles);
		ia->linearize(va->size());
		
		primitives::calculateTangents(va, ia, 0, IndexType(ia->primitivesCount()));
		
		_cubeMesh = rc->vertexBufferFactory().createVertexArrayObject("debug-cube", va,
			BufferDrawType_Static, ia, BufferDrawType_Static);
	}
	
	{
		vec2i planeDensity(5);
		VertexDeclaration decl(true, Usage_Position, Type_Vec3);
		decl.push_back(Usage_TexCoord0, Type_Vec3);
		decl.push_back(Usage_Normal, Type_Vec3);
		decl.push_back(Usage_Tangent, Type_Vec3);
		
		VertexArray::Pointer va = VertexArray::Pointer::create(decl, 0);
		
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat_16bit,
			primitives::indexCountForRegularMesh(planeDensity, PrimitiveType_TriangleStrips), PrimitiveType_TriangleStrips);
		
		primitives::createSquarePlane(va, -unitY, vec2(500.0f), planeDensity, vec3(0.0, -50.0f, 0.0f));
		primitives::buildTriangleStripIndexes(ia, planeDensity, 0, 0);
		primitives::calculateTangents(va, ia, 0, IndexType(ia->primitivesCount()));
		
		_planeMesh = rc->vertexBufferFactory().createVertexArrayObject("debug-plane", va,
			BufferDrawType_Static, ia, BufferDrawType_Static);
	}
}

void SceneRenderer::setScene(et::s3d::Scene::Pointer aScene)
{
	_allObjects.clear();
	
	_scene = aScene;
	
	auto meshes = _scene->childrenOfType(s3d::ElementType_SupportMesh);
	for (auto e : meshes)
		_allObjects.push_back(e);
	
	for (s3d::SupportMesh::Pointer& e : _allObjects)
	{
		auto mat = e->material();
		for (size_t i = MaterialParameter_AmbientMap; i < MaterialParameter_AmbientFactor; ++i)
		{
			if (mat->hasTexture(i))
			{
				Texture t = mat->getTexture(i);
				_rc->renderState().bindTexture(0, t);
				float maxAnisotropy = 0.0f;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
				glTexParameterf(t->target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
			}
		}
	}
	
	log::info("Scene set. %llu objects to render", static_cast<uint64_t>(_allObjects.size()));
}

void SceneRenderer::renderToGeometryBuffer(const et::Camera& cam)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setDepthMask(true);
	rs.setDepthTest(true);
	
	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	checkOpenGLError("glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)");
	
	rs.bindFramebuffer(_geometryBuffer);
	rn->clear(true, true);

	_geometryBuffer->setDrawBuffersCount(2);

	rs.bindProgram(programs.prepass);
	rs.bindVertexArray(_cubeMesh);
	rs.bindTexture(diffuseTextureUnit, _defaultTexture);
	rs.setWireframeRendering(true);
	
	programs.prepass->setCameraProperties(cam);
	for (const auto& l : _lightPositions)
	{
		programs.prepass->setTransformMatrix(translationMatrix(l));
		rn->drawAllElements(_cubeMesh->indexBuffer());
	}
	
	rs.setWireframeRendering(false);
	
	for (s3d::SupportMesh::Pointer& e : _allObjects)
	{
		if (cam.frustum().containsAABB(e->aabb()))
		{
			const auto& mat = e->material();
			programs.prepass->setTransformMatrix(e->finalTransform());
			rs.bindVertexArray(e->vertexArrayObject());
			
			if (mat->hasTexture(MaterialParameter_DiffuseMap))
				rs.bindTexture(diffuseTextureUnit, mat->getTexture(MaterialParameter_DiffuseMap));
			else
				rs.bindTexture(diffuseTextureUnit, _defaultTexture);
			
			if (mat->hasTexture(MaterialParameter_NormalMap))
				rs.bindTexture(normalTextureUnit, mat->getTexture(MaterialParameter_NormalMap));
			else
				rs.bindTexture(normalTextureUnit, _defaultNormalTexture);
			
			if (mat->hasTexture(MaterialParameter_TransparencyMap))
				rs.bindTexture(transparencyTextureUnit, mat->getTexture(MaterialParameter_TransparencyMap));
			else
				rs.bindTexture(transparencyTextureUnit, _defaultTexture);
			
			rn->drawElements(e->indexBuffer(), e->startIndex(), e->numIndexes());
		}
	}
// */
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	checkOpenGLError("glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE)");
}

void SceneRenderer::computeAmbientOcclusion(const et::Camera& cam)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setDepthMask(false);
	rs.setDepthTest(false);
	rs.bindFramebuffer(_downsampledBuffer);
	
	_downsampledBuffer->setCurrentRenderTarget(0);
	
	rs.bindTexture(noiseTextureUnit, _noiseTexture);
	rs.bindTexture(diffuseTextureUnit, _geometryBuffer->renderTarget(0));
	rs.bindTexture(normalTextureUnit, _geometryBuffer->renderTarget(1));
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	
	rs.bindProgram(programs.ambientOcclusion);
	programs.ambientOcclusion->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.ambientOcclusion->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	rn->fullscreenPass();
	
	rs.bindProgram(programs.ambientOcclusionBlur);
	programs.ambientOcclusionBlur->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.ambientOcclusionBlur->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	
	_downsampledBuffer->setCurrentRenderTarget(1);
	rs.bindTexture(diffuseTextureUnit, _downsampledBuffer->renderTarget(0));
	programs.ambientOcclusionBlur->setUniform("direction", _downsampledBuffer->renderTarget(0)->texel() * vec2(1.0, 0.0));
	rn->fullscreenPass();
	
	_downsampledBuffer->setCurrentRenderTarget(0);
	rs.bindTexture(diffuseTextureUnit, _downsampledBuffer->renderTarget(1));
	programs.ambientOcclusionBlur->setUniform("direction", _downsampledBuffer->renderTarget(0)->texel() * vec2(0.0, 1.0));
	rn->fullscreenPass();
	
	_downsampledBuffer->setCurrentRenderTarget(1);
	rs.bindTexture(diffuseTextureUnit, _downsampledBuffer->renderTarget(0));
	
	rs.bindProgram(programs.fxaa);
	programs.fxaa->setUniform("texel", _downsampledBuffer->renderTarget(0)->texel());
	rn->fullscreenPass();
}

void SceneRenderer::handlePressedKey(size_t key)
{
}

void SceneRenderer::render(const et::Camera& cam, const et::Camera& observer, bool obs)
{
	float currentTime = mainTimerPool()->actualTime();
	
	if (_updateTime == 0.0f)
		_updateTime = currentTime - 1.0f / 30.0f;
	
	float dt = currentTime - _updateTime;
	
	_updateTime = currentTime;
	
	if (_shouldSetPreviousProjectionMatrix)
	{
		_previousProjectionMatrix = cam.modelViewProjectionMatrix();
		_shouldSetPreviousProjectionMatrix = false;
	}
	
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setBlend(false);

	renderToGeometryBuffer(cam);
	computeAmbientOcclusion(cam);
	
	std::vector<vec3> viewSpaceLightPosition(_lightPositions.size());
	
	for (size_t i = 0; i < _lightPositions.size(); ++i)
	{
		_lightPositions.at(i).z = (i % 2 == 0) ? (std::cos(currentTime) * 225.0f) : (std::sin(currentTime) * 175.0f);
		viewSpaceLightPosition[i] = cam.modelViewMatrix() * _lightPositions.at(i);
	}
	
	rs.bindFramebuffer(_finalBuffers[_finalBufferIndex]);
	rs.bindProgram(programs.final);
		
	programs.final->setCameraProperties(cam);
	programs.final->setUniform<vec3>("lightPositions[0]", viewSpaceLightPosition.data(), viewSpaceLightPosition.size());
	programs.final->setUniform("lightsCount", viewSpaceLightPosition.size());
	programs.final->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.final->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	
	rs.bindTexture(diffuseTextureUnit, _geometryBuffer->renderTarget(0));
	rs.bindTexture(normalTextureUnit, _geometryBuffer->renderTarget(1));
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	rs.bindTexture(occlusionTextureUnit, _downsampledBuffer->renderTarget(1));
	rn->fullscreenPass();
	
	rs.bindFramebuffer(_finalBuffers[1 - _finalBufferIndex]);
	rs.bindProgram(programs.motionBlur);
	rs.bindTexture(diffuseTextureUnit, _finalBuffers[_finalBufferIndex]->renderTarget());
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	programs.motionBlur->setUniform("mModelViewInverseToPrevious", cam.inverseModelViewProjectionMatrix() * _previousProjectionMatrix);
	programs.motionBlur->setUniform("motionDistanceScale", 0.5f * (dt / baseFrameTime));
	rn->fullscreenPass();
	
	rs.bindDefaultFramebuffer();
	rs.bindProgram(programs.fxaa);
	rs.bindTexture(diffuseTextureUnit, _finalBuffers[1 - _finalBufferIndex]->renderTarget());
	programs.fxaa->setUniform("texel", _finalBuffers[1 - _finalBufferIndex]->renderTarget()->texel());
	rn->fullscreenPass();

	_previousProjectionMatrix = cam.modelViewProjectionMatrix();
	_finalBufferIndex = 1 - _finalBufferIndex;
}
