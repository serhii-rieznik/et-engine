//
//  MainController.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 21/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "MainController.h"

using namespace et;
using namespace rt;

const vec2 samplesPerScreen = vec2(16.0f);

et::IApplicationDelegate* Application::initApplicationDelegate()
	{ return new MainController(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "Raytrace"); }

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.contextSize = vec2i(512);
	p.contextBaseSize = p.contextSize;
	p.swapInterval = 1;
}

void MainController::updateTitle()
{
	application().setTitle("Bounces: " + intToStr(_bounces));
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_bounces = _productionBounces;
	
	updateTitle();
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
		log::info("FPS: %zu", info.averageFramePerSecond);
	});
	
	rc->renderState().setDepthMask(false);
	
	ObjectsCache localCache;
	_mainProgram = rc->programFactory().loadProgram("programs/main.program", localCache);
	_mainProgram->setUniform("noiseTexture", 0);
	
	_noise = rc->textureFactory().genNoiseTexture(vec2i(512), true, "tex_noise");
	
	_cameraAngles.updated.connect([this]()
	{
		_mainCamera.lookAt(70.0f * fromSpherical(_cameraAngles.value().y, _cameraAngles.value().x), vec3(5.0f, -30.0f, -30.0f));
	});
	
	_cameraAngles.setTargetValue(vec2(HALF_PI + 10.0 * TO_RADIANS, 15.5 * TO_RADIANS));
	_cameraAngles.finishInterpolation();
	_cameraAngles.run();
	_cameraAngles.updated.invoke();
	
	_gestures.pointerPressed.connect([this](et::PointerInputInfo)
	{
		restartOnlineRendering();
	});

	_gestures.pointerReleased.connect([this](et::PointerInputInfo)
	{
		restartOfflineRendering();
	});
	
	_gestures.drag.connect([this](et::vector2<float> d, unsigned long)
	{
		_cameraAngles.addTargetValue(0.1f * vec2(d.x, -d.y));
		_cameraAngles.finishInterpolation();
	});
	
	_scale = vec2(1.0f) / samplesPerScreen;
	_initialScale = _scale;
	
	_offset = vec2(-1.0f) + _scale;
	_initialOffset = _offset;

	float d = -40.0f;
	float r3 = 5.0f;
	float ballsPerRow = 6.0f;
	
	_lightPosition = vec3(0.0f, -0.5f * d, 0.75f * d);
	
	vec3 pos(d + 2.0f * r3);
	vec3 pos0 = pos;
	vec3 delta(-2.0f * d / ballsPerRow);
	
	for (int i = 0; i < 50; ++i)
	{
		_spheres.push_back(vec4(pos, r3));
		_sphereColors.push_back(vec4(randomFloat(0.5f, 1.5f), randomFloat(0.5f, 1.5f), randomFloat(0.5f, 1.5f), 0.0f));
		pos.x += delta.x;
		if (pos.x >= -d - r3)
		{
			pos.x = pos0.x;
			pos.z += delta.z;
			if (pos.z >= -d - r3)
			{
				pos.z = pos0.z;
				pos.y += delta.y;
			}
		}
	}

	// left
	_planes.push_back(vec4(normalize(vec3(1.0f, 0.25f, 0.0f)), d));
	_planeColors.push_back(3.0f * vec4(0.1f, 0.2f, 0.3f, 0.0f));
	
	// right
	_planes.push_back(vec4(normalize(vec3(-1.0f, 0.25f, 0.0f)), d));
	_planeColors.push_back(3.0f * vec4(0.3, 0.2f, 0.1f, 0.0f));
	
	// top
	_planes.push_back(vec4(0.0f, -1.0f, 0.0f, d));
	_planeColors.push_back(vec4(1.0f, 0.0f));
	
	// bottom
	_planes.push_back(vec4(0.0f, 1.0f, 0.0f, d));
	_planeColors.push_back(vec4(1.0f/3.0f, 0.0f));

	// back
	_planes.push_back(vec4(normalize(vec3(0.0f, 0.25f, 1.0f)), d));
	_planeColors.push_back(vec4(0.75f, 0.0f));
	
	// front
	_planes.push_back(vec4(normalize(vec3(0.0f, 0.25f, -1.0f)), d));
	_planeColors.push_back(vec4(3.0f/3.0f, 0.0f));
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	_mainCamera.perspectiveProjection(QUARTER_PI, vector2ToFloat(sz).aspect(), 1.0f, 1024.0f);
}

void MainController::performRender(et::RenderContext* rc)
{
	rc->renderState().bindTexture(0, _noise);
	rc->renderState().bindProgram(_mainProgram);
	
	_mainProgram->setCameraProperties(_mainCamera);
	_mainProgram->setPrimaryLightPosition(_lightPosition);
	_mainProgram->setUniform("scale", _scale);
	_mainProgram->setUniform("offset", _offset);
	_mainProgram->setUniform("mModelViewProjectionInverse", _mainCamera.inverseModelViewProjectionMatrix());
	_mainProgram->setUniform("maxBounces", _bounces);
	
	_mainProgram->setUniform("numPlanes", etMin(_planeColors.size(), _planes.size()));
	_mainProgram->setUniform<vec4>("planes[0]", _planes.data(), _planes.size());
	_mainProgram->setUniform<vec4>("planeColors[0]", _planeColors.data(), _planeColors.size());
	
	_mainProgram->setUniform("numSpheres", _spheres.size());
	_mainProgram->setUniform<vec4>("spheres[0]", _spheres.data(), _spheres.size());
	_mainProgram->setUniform<vec4>("sphereColors[0]", _sphereColors.data(), _sphereColors.size());
	
	rc->renderer()->fullscreenPass();
}

void MainController::render(et::RenderContext* rc)
{
	if (_interactiveRendering)
	{
		performRender(rc);
	}
	else
	{
		if (_shouldRender)
		{
			performRender(rc);
			
			_offset.x += 2.0f * _initialScale.x;
			if (_offset.x > 1.0f - _initialScale.x)
			{
				_offset.x = _initialOffset.x;
				_offset.y += 2.0f * _initialScale.y;
				if (_offset.y > 1.0f - _initialScale.y)
					_shouldRender = false;
			}
		}
	}
}

void MainController::idle(float)
{
	
}

void MainController::onKeyPressed(size_t key)
{
	if (key == ET_KEY_UP)
		_productionBounces = etMin(49, _productionBounces + 1);
	else if (key == ET_KEY_DOWN)
		_productionBounces = etMax(_previewBounces + 1, _productionBounces - 1);
	
	if (_bounces != _previewBounces)
		restartOfflineRendering();
}

void MainController::restartOnlineRendering()
{
	_shouldRender = true;
	_interactiveRendering = true;
	_cameraAngles.finishInterpolation();
	
	_bounces = _previewBounces;
	_offset = vec2(0.0f);
	_scale = vec2(1.0f);
	
	updateTitle();
}

void MainController::restartOfflineRendering()
{
	_shouldRender = true;
	_interactiveRendering = false;
	_cameraAngles.finishInterpolation();
	
	_bounces = _productionBounces;
	_offset = _initialOffset;
	_scale = _initialScale;
	
	updateTitle();
}
