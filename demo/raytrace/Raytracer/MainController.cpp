//
//  MainController.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 21/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <ctime>
#include <et/imaging/imagewriter.h>
#include "raytracer/Raytracer.h"
#include "MainController.h"

using namespace et;
using namespace rt;

const vec2i frameSize = vec2i(1280, 800) / 2;

const vec2i rectSize = vec2i(20);

const vec2i frameParts = vec2i(frameSize.x / rectSize.x, frameSize.y / rectSize.y);

const vec2 samplesPerScreen = vec2(static_cast<float>(frameParts.x), static_cast<float>(frameParts.y));

#if (ET_DEBUG)
	const size_t numThreads = 1;
#else
	const size_t numThreads = 4;
#endif

et::IApplicationDelegate* Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

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
	application().setTitle(intToStr(_scene.options.bounces) + " bounces, " + intToStr(_scene.options.samples) +
		" samples, remaining time: " + intToStr(_estimatedTime / 1000) + " sec. (" + intToStr(_estimatedTime / 60000) + " min.)");
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	application().pushSearchPath("..\\Data");
	application().pushSearchPath("Q:\\SDK\\");
	application().pushSearchPath("Q:\\SDK\\Models");
	application().pushSearchPath("Q:\\SDK\\Textures");
#endif
	
	_scene.options.bounces = _productionBounces;
	_scene.options.samples = _productionSamples;
	_scene.options.exposure = 1.0f;
	updateTitle();
	
	rc->renderState().setDepthMask(false);
	rc->renderState().setDepthTest(false);
	
	_scene.load(rc);
	
	ObjectsCache localCache;
	_mainProgram = rc->programFactory().loadProgram("programs/main.program", localCache);
	_mainProgram->setUniform("noiseTexture", 0);
	
	_noise = rc->textureFactory().genNoiseTexture(vec2i(512), true, "tex_noise");
	
	_textureData.resize(frameSize.square());
	_textureData.fill(255);
	
	_outputFunction = [this](const vec2i& pixel, const vec4& color) mutable
	{
		size_t index = pixel.x + pixel.y * frameSize.x;
		_textureData[index].x = static_cast<unsigned char>(255.0f * clamp(color.x, 0.0f, 1.0f));
		_textureData[index].y = static_cast<unsigned char>(255.0f * clamp(color.y, 0.0f, 1.0f));
		_textureData[index].z = static_cast<unsigned char>(255.0f * clamp(color.z, 0.0f, 1.0f));
	};
	
	BinaryDataStorage textureData(reinterpret_cast<unsigned char*>(_textureData.binary()), _textureData.dataSize());
	_result = rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA, frameSize,
		TextureFormat::RGBA, DataType::UnsignedChar, textureData, "result-texture");
	
	_cameraAngles.updated.connect([this]()
	{
		vec3 origin = 256.0f * fromSpherical(_cameraAngles.value().y, _cameraAngles.value().x);
		_scene.camera.lookAt(origin, 5.0f * unitY);
		
		if (!_enableGPURaytracing && (_scene.options.bounces == _previewBounces))
			startCPUTracing();
	});
	
	_cameraAngles.setTargetValue(vec2(-(HALF_PI + DEG_30), 37.5f * TO_RADIANS));
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
	
	for (size_t i = 0; i < numThreads; ++i)
		_threads.push_back(sharedObjectFactory().createObject<RaytraceThread>(this));
	
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
		sharedObjectFactory().deleteObject(t);
	}
}

bool MainController::fetchNewRenderRect(et::vec2i& origin, et::vec2i& size)
{
	CriticalSectionScope lock(_csLock);
	if (_renderRects.empty()) return false;
	
	int remainingRects = static_cast<int>(_renderRects.size());
	int randomOffset = remainingRects / 2 + rand() % (2 * frameParts.x) - frameParts.x;
	randomOffset = clamp(randomOffset, 0, remainingRects - 1);

	auto i = _renderRects.begin();
	std::advance(i, static_cast<size_t>(randomOffset));
	
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
	_scene.camera.perspectiveProjection(QUARTER_PI, vector2ToFloat(sz).aspect(), 1.0f, 2048.0f);
}

void MainController::performRender(et::RenderContext* rc)
{
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
		_productionBounces = etMin(size_t(49), _productionBounces + 1);
	else if (key == ET_KEY_DOWN)
		_productionBounces = etMax(_previewBounces + 1, _productionBounces - 1);
	
	if (_scene.options.bounces != _previewBounces)
		restartOfflineRendering();
}

void MainController::restartOnlineRendering()
{
	_rendering = true;

	_estimatedTime = 0;
	_maxElapsedTime = 0;

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

	_estimatedTime = 0;
	_maxElapsedTime = 0;

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

void MainController::renderFinished(uint64_t elapsedTime)
{
	CriticalSectionScope lock(_csLock);

	_maxElapsedTime += elapsedTime;

	size_t remainingRects = _renderRects.size();
	size_t totalRects = frameParts.square();

	if (totalRects != remainingRects)
	{
		uint64_t averageTime = _maxElapsedTime / (totalRects - remainingRects);
		_estimatedTime = remainingRects * averageTime;
		updateTitle();
	}

	if (!_renderRects.empty()) return;
	
	for (auto rt : _threads)
	{
		if (rt->rendering())
			return;
	}
	
	auto renderTime = floatToStr(mainTimerPool()->actualTime() - _startTime, 2);
	
	std::string fn = application().environment().applicationDocumentsFolder() + "result (" +
		renderTime + " sec, " + intToStr(_scene.options.samples) + " samples per pixel).png";
	
	writeImageToFile(fn, BinaryDataStorage(reinterpret_cast<unsigned char*>(_textureData.binary()),
		_textureData.dataSize()), frameSize, 4, 8, ImageFormat_PNG, true);
}
