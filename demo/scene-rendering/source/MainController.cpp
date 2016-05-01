#include <et/input/input.h>
#include <et/rendering/rendercontext.h>
#include "MainController.h"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& params)
{
    params.context.size = vec2i(1024, 768);
}

void MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
    params.multisamplingQuality = MultisamplingQuality::Best;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	application().pushRelativeSearchPath("..");
	application().pushRelativeSearchPath("..\\..");
	application().pushRelativeSearchPath("..\\..\\..");
	application().pushSearchPath("Q:\\SDK\\Models\\");
	application().pushSearchPath("Q:\\SDK\\");
#elif (ET_PLATFORM_MAC)
	application().pushSearchPath("/Volumes/Development/SDK/");
	application().pushSearchPath("/Volumes/Development/SDK/Models/");
#endif

	rc->renderState().setClearColor(vec4(0.25f, 0.0f));
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
//		log::info("Rendering stats: %lu fps, %lu polys, %lu draw calls", info.averageFramePerSecond,
//			info.averagePolygonsPerSecond, info.averageDIPPerSecond);
	});
    
    _camera.lookAt(vec3(500.0f));
    _cameraController = et::CameraMovingController::Pointer::create(_camera, true);
    _cameraController->setIntepolationRate(10.0f);
    _cameraController->setMovementSpeed(vec3(100.0f));
    _cameraController->synchronize(_camera);
    _cameraController->startUpdates();
		
	_loader.init(rc);
	_renderer.init(rc);
	
	auto loadedScene = _loader.loadFromFile(application().resolveFileName("media/material-test.obj"));
	_renderer.setScene(loadedScene);
}

 void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
    vec2 fSz = vector2ToFloat(sz);
    _camera.perspectiveProjection(DEG_45, fSz.aspect(), 1.0f, 1024.0f);
}

void MainController::render(et::RenderContext* rc)
{
	_renderer.render(_camera, _camera);
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier("com.cheetek.scenerendering", "Cheetek", "Scene Rendering Demo"); }
