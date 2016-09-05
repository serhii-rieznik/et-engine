#include <et/input/input.h>
#include <et/rendering/rendercontext.h>
#include "MainController.h"

void demo::MainController::setApplicationParameters(et::ApplicationParameters& params)
{
	params.renderingAPI = et::RenderingAPI::Metal;
	params.context.size = et::vec2i(1024, 640);
}

void demo::MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	params.multisamplingQuality = et::MultisamplingQuality::Best;
}

void demo::MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	application().pushRelativeSearchPath("..");
	application().pushRelativeSearchPath("..\\..");
	application().pushRelativeSearchPath("..\\..\\..");
	application().pushSearchPath("Q:\\SDK\\Models\\");
	application().pushSearchPath("Q:\\SDK\\");
#elif (ET_PLATFORM_MAC)
	et::application().pushSearchPath("/Volumes/Development/SDK/");
	et::application().pushSearchPath("/Volumes/Development/SDK/Models/");
#endif

    _camera.lookAt(et::vec3(500.0f));
    _cameraController = et::CameraMovingController::Pointer::create(_camera, true);
    _cameraController->setIntepolationRate(10.0f);
    _cameraController->setMovementSpeed(et::vec3(100.0f));
    _cameraController->synchronize(_camera);
    _cameraController->startUpdates();
		
	_loader.init(rc);
	_renderer.init(rc);
	
	auto loadedScene = _loader.loadFromFile(et::application().resolveFileName("media/material-test.obj"));
	_renderer.setScene(loadedScene);
}

void demo::MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	et::vec2 fSz = vector2ToFloat(sz);
    _camera.perspectiveProjection(DEG_45, fSz.aspect(), 1.0f, 1024.0f);
}

void demo::MainController::render(et::RenderContext* rc)
{
	_renderer.render(_camera, _camera);
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<demo::MainController>(); }

et::ApplicationIdentifier demo::MainController::applicationIdentifier() const
	{ return et::ApplicationIdentifier("com.cheetek.scenerendering", "Cheetek", "Scene Rendering Demo"); }
