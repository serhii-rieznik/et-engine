#include <et/models/objloader.h>
#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include <et/json/json.h>
#include "maincontroller.hpp"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality_None;
	p.contextBaseSize = vec2i(1024, 640);
	p.contextSize = p.contextBaseSize;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	srand(static_cast<unsigned int>(time(nullptr)));
	
	ObjectsCache localCache;

	vec2i textureSize = rc->sizei();

	_textureData.resize(textureSize.square() * sizeof(vec4));
	_textureData.fill(0);

	_texture = rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA32F, textureSize,
		TextureFormat::RGBA, DataType::Float, _textureData, "output-texture");

#if (ET_PLATFORM_WIN)
	application().pushSearchPath("Q:\\SDK\\Models\\");
#endif
	
	auto modelName = application().resolveFileName("media/cornellbox.obj");
	auto configName = application().resolveFileName("config/config.json");

	_scene = s3d::Scene::Pointer::create();
	OBJLoader loader(modelName, OBJLoader::Option_CalculateTangents);
	auto model = loader.load(rc, _scene->storage(), localCache);
	model->setParent(_scene.ptr());
	
	ValueClass vc = ValueClass_Invalid;
	Dictionary options = json::deserialize(loadTextFile(configName), vc);
	ET_ASSERT(vc == ValueClass_Dictionary);

	const vec3 lookPoint = vec3(0.0f, 1.0f, 0.0f);
	const vec3 offset = vec3(0.0f, 1.0f, 0.0f);
	float cameraPhi = HALF_PI;
	float cameraTheta = 0.0f;
	float cameraDistance = options.floatForKey("initial-camera-distance", 3.0f)->content;
	_camera.perspectiveProjection(DEG_60, vector2ToFloat(textureSize).aspect(), 1.0f, 1024.0f);
	_camera.lookAt(cameraDistance * fromSpherical(cameraTheta, cameraPhi) + offset, lookPoint);
	
	Raytrace::Options rtOptions;
	rtOptions.maxRecursionDepth = options.integerForKey("max-recursion-depth", 8)->content;
	rtOptions.raysPerPixel = options.integerForKey("rays-per-pixel", 32)->content;
	_rt.setOptions(rtOptions);

	_rt.setOutputMethod([this](const vec2i& pixel, const vec4& color)
	{
		DataStorage<vec4> vec4data(reinterpret_cast<vec4*>(_textureData.binary()), _textureData.dataSize());
		vec4data[pixel.x + pixel.y * _texture->size().x] = color;
	});

	_rt.perform(_scene, _camera, _texture->size());

	_gestures.click.connect([this](const PointerInputInfo& p)
	{
		_rt.performAtPoint(_scene, _camera, _texture->size(), vec2i(int(p.pos.x), int(p.pos.y)));
	});
}

void MainController::applicationWillTerminate()
{
	_rt.stop();
}

void MainController::render(et::RenderContext* rc)
{
	_texture->updateDataDirectly(rc, _texture->size(), _textureData.binary(), _textureData.dataSize());
	rc->renderer()->clear(true, true);
	rc->renderer()->renderFullscreenTexture(_texture);
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return et::ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "RT demo"); }
