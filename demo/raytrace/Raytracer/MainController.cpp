//
//  MainController.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 21/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/imaging/imagewriter.h>
#include "Raytracer.h"
#include "MainController.h"

using namespace et;
using namespace rt;

const vec2i frameSize = vec2i(1920, 1200);

const vec2i rectSize = vec2i(80);

const vec2i frameParts = vec2i(frameSize.x / rectSize.x, frameSize.y / rectSize.y);

const vec2 samplesPerScreen = vec2(static_cast<float>(frameParts.x), static_cast<float>(frameParts.y));

et::IApplicationDelegate* Application::initApplicationDelegate()
	{ return new MainController(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "Raytrace"); }

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	srand(static_cast<unsigned int>(time(nullptr)));
	
	p.contextSize = frameSize;
	p.contextBaseSize = p.contextSize;
	p.swapInterval = 0;
}

void MainController::updateTitle()
{
	application().setTitle("Bounces: " + intToStr(_scene.options.bounces));
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	application().pushSearchPath("..\\Data");
#endif
	
	_scene.options.bounces = _productionBounces;
	_scene.options.samples = _productionSamples;
	updateTitle();
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
		log::info("FPS: %lld", (int64_t)info.averageFramePerSecond);
	});
	
	rc->renderState().setDepthMask(false);
	
	ObjectsCache localCache;
	_mainProgram = rc->programFactory().loadProgram("programs/main.program", localCache);
	_mainProgram->setUniform("noiseTexture", 0);
	
	_noise = rc->textureFactory().genNoiseTexture(vec2i(512), true, "tex_noise");
	_result = rc->textureFactory().genNoiseTexture(frameSize, true, "result-texture");
	
	_textureData.resize(frameSize.square());
	_textureData.fill(255);
	
	_outputFunction = [this](const vec2i& pixel, const vec4& color)
	{
		size_t index = pixel.x + pixel.y * frameSize.x;
		_textureData[index].x = static_cast<unsigned char>(255.0f * clamp(color.x, 0.0f, 1.0f));
		_textureData[index].y = static_cast<unsigned char>(255.0f * clamp(color.y, 0.0f, 1.0f));
		_textureData[index].z = static_cast<unsigned char>(255.0f * clamp(color.z, 0.0f, 1.0f));
	};
	
	_result = rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, frameSize, GL_RGBA, GL_UNSIGNED_BYTE,
		BinaryDataStorage(reinterpret_cast<unsigned char*>(_textureData.binary()), _textureData.dataSize()), "result-texture");
	
	_cameraAngles.updated.connect([this]()
	{
		_scene.camera.lookAt(70.0f * fromSpherical(_cameraAngles.value().y, _cameraAngles.value().x) - vec3(0.0f, 5.0f, 0.0f),
			vec3(7.5f, -16.0f, -0.0f));
		
		if (!_enableGPURaytracing && (_scene.options.bounces == _previewBounces))
			startCPUTracing();
	});
	
	_cameraAngles.setTargetValue(vec2(-HALF_PI + 10.0f * TO_RADIANS, 15.5f * TO_RADIANS));
	_cameraAngles.finishInterpolation();
	_cameraAngles.run();
	_cameraAngles.updated.invoke();
	
	_gestures.pointerPressed.connect([this](et::PointerInputInfo)
		{ restartOnlineRendering(); });

	_gestures.pointerReleased.connect([this](et::PointerInputInfo)
		{ restartOfflineRendering(); });
	
	_gestures.drag.connect([this](et::vector2<float> d, unsigned long)
	{
		_cameraAngles.addTargetValue(0.1f * vec2(d.x, -d.y));
		_cameraAngles.finishInterpolation();
	});
	
	for (size_t i = 0; i < 4; ++i)
		_threads.emplace_back(new RaytraceThread(this));
	
	_scale = vec2(1.0f) / samplesPerScreen;
	_offset = vec2(-1.0f) + _scale;
	
	_initialScale = _scale;
	_initialOffset = _offset;
	
	if (!_enableGPURaytracing)
		restartOfflineRendering();
}

void MainController::applicationWillTerminate()
{
	for (auto t : _threads)
	{
		t->stop();
		t->waitForTermination();
		
		delete t;
	}
}

bool MainController::fetchNewRenderRect(et::vec2i& origin, et::vec2i& size)
{
	CriticalSectionScope lock(_csLock);
	if (_renderRects.empty()) return false;
	
	auto i = _renderRects.begin();
	std::advance(i, rand() % _renderRects.size());
	
	origin = i->origin();
	size = i->size();
	
	_renderRects.erase(i);
	
	return true;
}

et::vec2i MainController::imageSize()
{
	return frameSize;
}

const RaytraceScene& MainController::scene()
{
	return _scene;
}

OutputFunction MainController::outputFunction()
{
	return _outputFunction;
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	_scene.camera.perspectiveProjection(QUARTER_PI, vector2ToFloat(sz).aspect(), 1.0f, 1024.0f);
}

void MainController::performRender(et::RenderContext* rc)
{
	rc->renderState().bindTexture(0, _noise);
	rc->renderState().bindProgram(_mainProgram);
	
	_mainProgram->setCameraProperties(_scene.camera);
	
	_mainProgram->setUniform("scale", _scale);
	_mainProgram->setUniform("offset", _offset);
	_mainProgram->setUniform("mModelViewProjectionInverse", _scene.camera.inverseModelViewProjectionMatrix());
	
	_mainProgram->setUniform("maxBounces", _scene.options.bounces);
	_mainProgram->setUniform("maxSamples", _scene.options.samples);
	_mainProgram->setUniform("exposure", _scene.options.exposure);
	
	_mainProgram->setUniform("lightSphere", _scene.lightSphere);
	_mainProgram->setUniform("lightColor", _scene.lightColor);
	
	_mainProgram->setUniform("numPlanes", etMin(_scene.planeColors.size(), _scene.planes.size()));
	_mainProgram->setUniform<vec4>("planes[0]", _scene.planes.data(), _scene.planes.size());
	_mainProgram->setUniform<vec4>("planeColors[0]", _scene.planeColors.data(), _scene.planeColors.size());
	
	_mainProgram->setUniform("numSpheres", _scene.spheres.size());
	_mainProgram->setUniform<vec4>("spheres[0]", _scene.spheres.data(), _scene.spheres.size());
	_mainProgram->setUniform<vec4>("sphereColors[0]", _scene.sphereColors.data(), _scene.sphereColors.size());
	
	rc->renderer()->fullscreenPass();
}

void MainController::render(et::RenderContext* rc)
{
	if (_enableGPURaytracing)
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
	else
	{
		rc->renderState().bindTexture(0, _result);
		_result->updateDataDirectly(rc, frameSize, _textureData.binary(), _textureData.dataSize());
		rc->renderer()->renderFullscreenTexture(_result);
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
	
	if (_scene.options.bounces != _previewBounces)
		restartOfflineRendering();
}

void MainController::restartOnlineRendering()
{
	_rendering = true;
	_startTime = mainTimerPool()->actualTime();
	_scene.options.bounces = _previewBounces;
	_scene.options.samples = _previewSamples;
	_cameraAngles.finishInterpolation();
	
	if (_enableGPURaytracing)
	{
		_shouldRender = true;
		_interactiveRendering = true;
		
		_offset = vec2(0.0f);
		_scale = vec2(1.0f);
	}
	else
	{
		startCPUTracing();
	}
	
	updateTitle();
}

void MainController::restartOfflineRendering()
{
	_rendering = true;
	_startTime = mainTimerPool()->actualTime();
	_cameraAngles.finishInterpolation();
	_scene.options.bounces = _productionBounces;
	_scene.options.samples = _productionSamples;

	if (_enableGPURaytracing)
	{
		_shouldRender = true;
		_interactiveRendering = false;
		_offset = _initialOffset;
		_scale = _initialScale;
	}
	else
	{
		startCPUTracing();
	}
	
	updateTitle();
}

void MainController::startCPUTracing()
{
	CriticalSectionScope lock(_csLock);
	_renderRects.clear();
	
	for (int y = 0; y < frameParts.y; ++y)
	{
		for (int x = 0; x < frameParts.x; ++x)
			_renderRects.push_back(recti(vec2i(x, y) * rectSize, rectSize));
	}
}

bool MainController::shouldAntialias()
{
	return (_scene.options.samples != _previewSamples);
}

void MainController::renderFinished()
{
	CriticalSectionScope lock(_csLock);
	if (!_renderRects.empty()) return;
	
	for (auto rt : _threads)
	{
		if (rt->rendering())
			return;
	}
	
	auto renderTime = floatToStr(mainTimerPool()->actualTime() - _startTime, 2.0f);
	
	std::string fn = application().environment().applicationDocumentsFolder() + "result (" +
		renderTime + " sec, " + intToStr(4 * _scene.options.samples) + " samples per pixel).png";
	
	ImageWriter::writeImageToFile(fn, BinaryDataStorage(reinterpret_cast<unsigned char*>(_textureData.binary()),
		_textureData.dataSize()), frameSize, 4, 8, ImageFormat_PNG, true);
}
