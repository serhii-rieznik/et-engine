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

const vec2i frameSize = vec2i(1280, 800);

#if (ET_DEBUG)
	const size_t numThreads = 1;
#else
	const size_t numThreads = 4;
#endif

const float cameraDistance = 173.5f;

const vec3 cameraOffset = vec3(0.0f, 1.0f, 0.0f);

const vec2 cameraInitialAngles = vec2(HALF_PI, DEG_30);

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
#elif (ET_PLATFORM_MAC)
	application().pushSearchPath("/Volumes/Development/SDK");
	application().pushSearchPath("/Volumes/Development/SDK/Models");
	application().pushSearchPath("/Volumes/Development/SDK/Textures");
#endif
	
	_scene.options.bounces = _productionBounces;
	_scene.options.samples = _productionSamples;
	_scene.options.exposure = 1.0f;
	updateTitle();
	
	rc->renderState().setDepthMask(false);
	rc->renderState().setDepthTest(false);
	
	_scene.load(rc);
	
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
	
	_cameraAngles.setValue(cameraInitialAngles);
	_cameraAngles.setTargetValue(cameraInitialAngles);
	_cameraAngles.finishInterpolation();
	
	_cameraAngles.updated.connect([this]()
	{
		updateCamera();
		startRender(true);
	});
	
	updateCamera();
	_cameraAngles.run();
	
	_gestures.pointerPressed.connect([this](et::PointerInputInfo)
		{ startRender(true); });

	_gestures.pointerReleased.connect([this](et::PointerInputInfo)
		{ startRender(false); });
	
	_gestures.drag.connect([this](et::vector2<float> d, unsigned long)
	{
		_cameraAngles.addTargetValue(0.1f * vec2(d.x, -d.y));
		_cameraAngles.finishInterpolation();
	});
	
	for (size_t i = 0; i < numThreads; ++i)
		_threads.push_back(sharedObjectFactory().createObject<RaytraceThread>(this));
		
	Invocation([this](){ startRender(false); }).invokeInMainRunLoop();
}

void MainController::updateCamera()
{
	vec3 origin = cameraDistance * fromSpherical(_cameraAngles.value().y, _cameraAngles.value().x);
	_scene.camera.lookAt(cameraOffset + origin, cameraOffset);
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

bool MainController::fetchNewRenderRect(et::vec2i& origin, et::vec2i& size, bool& preview)
{
	CriticalSectionScope lock(_csLock);
	
	if (_renderRectIndex >= _renderRects.size()) return false;
	
	preview = (_scene.options.bounces == _previewBounces);
	
	const auto& rr = _renderRects.at(_renderRectIndex++);
	origin = rr.r.origin();
	size = rr.r.size();
	
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

void MainController::render(et::RenderContext* rc)
{
	rc->renderState().bindTexture(0, _result);
	_result->updateDataDirectly(rc, frameSize, _textureData.binary(), _textureData.dataSize());
	rc->renderer()->renderFullscreenTexture(_result);
}

void MainController::onKeyPressed(size_t key)
{
	if (key == ET_KEY_UP)
	{
		_productionBounces = etMin(size_t(49), _productionBounces + 1);
		if (_scene.options.bounces != _previewBounces)
			startRender(false);
	}
	else if (key == ET_KEY_DOWN)
	{
		_productionBounces = etMax(_previewBounces + 1, _productionBounces - 1);
		if (_scene.options.bounces != _previewBounces)
			startRender(false);
	}
}

void MainController::startRender(bool preview)
{
	CriticalSectionScope lock(_csLock);
	
	_rendering = true;
	
	_startTime = mainTimerPool()->actualTime();
	_scene.options.bounces = preview ? _previewBounces : _productionBounces;
	_scene.options.samples = preview ? _previewSamples : _productionSamples;
	
	updateTitle();
	
	_renderRects = estimateRenderRects(_scene, frameSize, preview);
	
	_renderRectIndex = 0;
	_estimatedTime = 0;
	_maxElapsedTime = 0;
}

void MainController::renderFinished(uint64_t elapsedTime)
{
	CriticalSectionScope lock(_csLock);
	
	_maxElapsedTime += elapsedTime;

	size_t remainingRects = (_renderRects.size() - _renderRectIndex);
	if (remainingRects == 0)
	{
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
	else if (_renderRectIndex > 0)
	{
		uint64_t averageTime = _maxElapsedTime / _renderRectIndex;
		_estimatedTime = remainingRects * averageTime;
		updateTitle();
	}
}
