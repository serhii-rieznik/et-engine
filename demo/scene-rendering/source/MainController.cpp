#include <et/input/input.h>
#include <et/core/conversion.h>
#include <et/core/json.h>
#include <et/rendering/rendercontext.h>
#include <et/core/hardware.h>
#include "MainController.h"

void demo::MainController::setApplicationParameters(et::ApplicationParameters& params)
{
#if (ET_PLATFORM_WIN)
	params.renderingAPI = et::RenderingAPI::Vulkan;
#elif (ET_PLATFORM_MAC)
    params.renderingAPI = et::RenderingAPI::Metal;
#endif
	
    params.context.size = 4 * et::currentScreen().frame.size() / 5;
	params.context.style |= et::ContextOptions::Style::Sizable;
}

void demo::MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	params.multisamplingQuality = et::MultisamplingQuality::Best;
}

void demo::MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
    et::application().pushSearchPath("..");
    et::application().pushSearchPath("..\\..");
    et::application().pushSearchPath("..\\..\\..");
    et::application().pushSearchPath("..\\..\\..\\..");
#endif

	_loader.init(rc);

	auto configName = et::application().resolveFileName("config/config.json");
	et::VariantClass vc = et::VariantClass::Invalid;
	_options = et::json::deserialize(et::loadTextFile(configName), vc);
	ET_ASSERT(vc == et::VariantClass::Dictionary);

	if (_options.hasKey("reference"))
	{
		vc = et::VariantClass::Invalid;
		configName = et::application().resolveFileName("config/" + _options.stringForKey("reference")->content);
		et::Dictionary reference = et::json::deserialize(et::loadTextFile(configName), vc);
		ET_ASSERT(vc == et::VariantClass::Dictionary);

		for (auto& kv : reference->content)
		{
			_options.setObjectForKey(kv.first, kv.second);
		}
	}

	auto modelName = et::application().resolveFileName(_options.stringForKey("model-name")->content);

	const et::vec3 lookPoint = et::arrayToVec3(_options.arrayForKey("camera-view-point"));
	const et::vec3 offset = et::arrayToVec3(_options.arrayForKey("camera-offset"));
	_cameraFOV = _options.floatForKey("camera-fov", 60.0f)->content * TO_RADIANS;
	float cameraPhi = _options.floatForKey("camera-phi", 0.0f)->content * TO_RADIANS;
	float cameraTheta = _options.floatForKey("camera-theta", 0.0f)->content * TO_RADIANS;
	float cameraDistance = _options.floatForKey("camera-distance", 3.0f)->content;

	_camera = et::Camera::Pointer::create();
	_camera->lookAt(cameraDistance * et::fromSpherical(cameraTheta, cameraPhi) + offset, lookPoint);

	et::log::info("Camera position: %f, %f, %f", _camera->position().x, _camera->position().y, _camera->position().z);

	_cameraController = et::CameraMovingController::Pointer::create(_camera, true);
	_cameraController->setIntepolationRate(10.0f);
	_cameraController->setMovementSpeed(et::vec3(100.0f));
	_cameraController->synchronize(_camera);
	_cameraController->startUpdates();

	_scene = _loader.loadFromFile(modelName);

	applicationWillResizeContext(rc->size());

	_fpsTimer.expired.connect([this](et::NotifyTimer*){
		et::log::info("%u fps", _framesRendered);
		_framesRendered = 0;
	});
	_fpsTimer.start(et::mainTimerPool(), 1.0f, -1);
	et::application().setFrameRateLimit(0);
}

void demo::MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	et::vec2 fSz = vector2ToFloat(sz);
    _camera->perspectiveProjection(_cameraFOV, fSz.aspect(), 1.0f, 2048.0f);
}

void demo::MainController::render(et::RenderContext* rc)
{
	_renderer.render(rc->renderer(), _scene.reference(), _camera);
	++_framesRendered;
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<demo::MainController>(); }

et::ApplicationIdentifier demo::MainController::applicationIdentifier() const
	{ return et::ApplicationIdentifier("com.cheetek.scenerendering", "Cheetek", "Scene Rendering Demo"); }
