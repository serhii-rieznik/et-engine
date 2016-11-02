#include <et/core/json.h>
#include <et/core/conversion.h>
#include <et/core/hardware.h>
#include <et/camera/camera.h>
#include <et/scene3d/objloader.h>
#include <et/rendering/base/primitives.h>
#include <et/rendering/base/helpers.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/texturedescription.h>
#include "maincontroller.hpp"

namespace demo
{

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
#if (ET_PLATFORM_WIN)
    p.renderingAPI = et::RenderingAPI::Vulkan;
#elif (ET_PLATFORM_MAC)
    p.renderingAPI = et::RenderingAPI::Metal;
#endif
    
	p.context.style |= et::ContextOptions::Style::Sizable;
    p.context.supportsHighResolution = true;
    p.context.size = 4 * et::currentScreen().frame.size() / 5;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
    p.multisamplingQuality = et::MultisamplingQuality::None;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_rc = rc;
	srand(static_cast<unsigned int>(time(nullptr)));

    et::ObjectsCache localCache;

#if (ET_PLATFORM_WIN)
	et::application().pushSearchPath("..");
	et::application().pushSearchPath("..\\..");
	et::application().pushSearchPath("..\\..\\..");
	et::application().pushSearchPath("..\\..\\..\\..");
	et::application().pushSearchPath("Q:\\SDK\\Textures");
#elif (ET_PLATFORM_MAC)
	et::application().pushSearchPath("/Volumes/Development/SDK/Textures");
#endif

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

	et::VertexDeclaration decl(true, et::VertexAttributeUsage::Position, et::DataType::Vec3);
	decl.push_back(et::VertexAttributeUsage::Normal, et::DataType::Vec3);
	decl.push_back(et::VertexAttributeUsage::TexCoord0, et::DataType::Vec2);

	if (!et::fileExists(modelName))
	{
		ET_FAIL("Unable to load specified model");
	}

	_scene = et::s3d::Scene::Pointer::create();
	
	et::OBJLoader loader(modelName, et::OBJLoader::Option_CalculateTangents);
	auto model = loader.load(rc->renderer(), _scene->storage(), localCache);
	model->setParent(_scene.pointer());

	et::Raytrace::Options rtOptions;
	rtOptions.raysPerPixel = static_cast<uint32_t>(_options.integerForKey("rays-per-pixel", 32)->content);
	rtOptions.maxKDTreeDepth = static_cast<uint32_t>(_options.integerForKey("kd-tree-max-depth", 4)->content);
	rtOptions.maxPathLength = static_cast<uint32_t>(_options.integerForKey("max-path-length", 1)->content);
	rtOptions.renderRegionSize = static_cast<uint32_t>(_options.integerForKey("render-region-size", 32)->content);
	rtOptions.threads = static_cast<uint32_t>(_options.integerForKey("threads", 0ll)->content);
	rtOptions.renderKDTree = _options.integerForKey("render-kd-tree", 0ll)->content != 0;

	if ((rtOptions.maxPathLength == 0) || (rtOptions.maxPathLength > et::rt::PathTraceIntegrator::MaxTraverseDepth))
	{
		rtOptions.maxPathLength = et::rt::PathTraceIntegrator::MaxTraverseDepth;
	}

	if (_options.stringForKey("method")->content == "forward")
	{
		rtOptions.method = et::Raytrace::Method::LightTracing;
	}
	else
	{
		rtOptions.method = et::Raytrace::Method::PathTracing;
	}
	_rt.setOptions(rtOptions);

	_rt.setOutputMethod([this](const et::vec2i& pixel, const et::vec4& color)
	{
		if ((pixel.x >= 0) && (pixel.y >= 0) && (pixel.x < _texture->size().x) &&  (pixel.y < _texture->size().y))
        {
            int pos = pixel.x + pixel.y * _texture->size().x;
            _textureData[pos] = mix(_textureData[pos], color, color.w);
            ET_ASSERT(!isnan(_textureData[pos].x));
            ET_ASSERT(!isnan(_textureData[pos].y));
            ET_ASSERT(!isnan(_textureData[pos].z));
            ET_ASSERT(!isnan(_textureData[pos].w));
        }
	});

	auto integrator = _options.stringForKey("integrator", "path-trace")->content;
	if (integrator == "ao")
	{
		_rt.setIntegrator(et::rt::AmbientOcclusionIntegrator::Pointer::create());
	}
	else if (integrator == "hack-ao")
	{
		_rt.setIntegrator(et::rt::AmbientOcclusionHackIntegrator::Pointer::create());
	}
	else if (integrator == "normals")
	{
		_rt.setIntegrator(et::rt::NormalsIntegrator::Pointer::create());
	}
	else if (integrator == "fresnel")
	{
		_rt.setIntegrator(et::rt::FresnelIntegrator::Pointer::create());
	}
	else
	{
		_rt.setIntegrator(et::rt::PathTraceIntegrator::Pointer::create());
	}

	et::rt::float4 envColor(et::arrayToVec4(_options.arrayForKey("env-color")));

	auto envMap = _options.stringForKey("env-map")->content;
	if (envMap.empty() == false)
		envMap = et::application().resolveFileName(envMap);

	if (et::fileExists(envMap))
	{
        et::TextureDescription::Pointer texture = et::TextureDescription::Pointer::create(envMap);
		_rt.setEnvironmentSampler(et::rt::EnvironmentEquirectangularMapSampler::Pointer::create(texture, envColor));
	}
	else
	{
        auto envType = _options.stringForKey("env-type", "uniform")->content;
        if (envType == "directional")
        {
            et::rt::float4 light(et::arrayToVec4(_options.arrayForKey("light-direction")));
            _rt.setEnvironmentSampler(et::rt::DirectionalLightSampler::Pointer::create(light, envColor));
        }
        else
        {
            _rt.setEnvironmentSampler(et::rt::EnvironmentColorSampler::Pointer::create(envColor));
        }
	}

	et::Input::instance().keyPressed.connect([this](size_t key) {
		if (key == ET_KEY_SPACE)
			start();
	});

	_gestures.click.connect([this](const et::PointerInputInfo& p) {
		et::vec2i pixel(int(p.pos.x), int(p.pos.y));
		et::vec4 color = _rt.performAtPoint(_scene, _camera, _texture->size(), pixel);
		_rt.output(et::vec2i(pixel.x, _texture->size().y - pixel.y), color);
	});

	et::Input::instance().keyPressed.invokeInMainRunLoop(ET_KEY_SPACE);
}

void MainController::start()
{
	_rt.stop();

	et::vec2i textureSize = _rc->size();

	_textureData.resize(textureSize.square());
	_textureData.fill(0);

    et::TextureDescription::Pointer desc = et::TextureDescription::Pointer::create();
    desc->target = et::TextureTarget::Texture_2D;
    desc->format = et::TextureFormat::RGBA32F;
    desc->size = textureSize;
    desc->data = et::BinaryDataStorage(reinterpret_cast<unsigned char*>(_textureData.data()), _textureData.dataSize());
    _texture = _rc->renderer()->createTexture(desc);

	_mainPass = _rc->renderer()->allocateRenderPass(et::RenderPass::ConstructionInfo());
	_fullscreenQuad = et::renderhelper::createFullscreenRenderBatch(_texture);

	const et::vec3 lookPoint = arrayToVec3(_options.arrayForKey("camera-view-point"));
	const et::vec3 offset = arrayToVec3(_options.arrayForKey("camera-offset"));
	float cameraFOV = _options.floatForKey("camera-fov", 60.0f)->content * TO_RADIANS;
	float cameraPhi = _options.floatForKey("camera-phi", 0.0f)->content * TO_RADIANS;
	float cameraTheta = _options.floatForKey("camera-theta", 0.0f)->content * TO_RADIANS;
	float cameraDistance = _options.floatForKey("camera-distance", 3.0f)->content;
	_camera.perspectiveProjection(cameraFOV, vector2ToFloat(textureSize).aspect(), 0.1f, 2048.0f);
	_camera.lookAt(cameraDistance * et::fromSpherical(cameraTheta, cameraPhi) + offset, lookPoint);
    et::log::info("Camera position: %f, %f, %f", _camera.position().x, _camera.position().y, _camera.position().z);

	_rt.perform(_scene, _camera, _texture->size());
}

void MainController::applicationWillTerminate()
{
	_rt.stop();
}

void MainController::render(et::RenderContext* rc)
{
	if (_texture.valid())
	{
        _texture->setImageData(et::BinaryDataStorage(reinterpret_cast<uint8_t*>(_textureData.data()), _textureData.dataSize()));
	}

	_mainPass->begin();
	_mainPass->pushRenderBatch(_fullscreenQuad);
	_mainPass->end();

	rc->renderer()->submitRenderPass(_mainPass);
}
    
et::ApplicationIdentifier demo::MainController::applicationIdentifier() const
{
    return et::ApplicationIdentifier(et::applicationIdentifierForCurrentProject(), "Cheetek", "RT demo");
}
    
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
{
    return sharedObjectFactory().createObject<demo::MainController>();
}

