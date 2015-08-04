#include <et/models/objloader.h>
#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include "maincontroller.hpp"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality_None;
	p.contextBaseSize = vec2i(1024, 768);
	p.contextSize = p.contextBaseSize;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	ObjectsCache localCache;

	vec2i textureSize = rc->sizei();

	_textureData.resize(textureSize.square() * sizeof(vec4));
	_textureData.fill(0);

	_texture = rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA32F, textureSize,
		TextureFormat::RGBA, DataType::Float, _textureData, "output-texture");

	application().pushSearchPath("Q:\\SDK\\Models\\");
	auto modelName = application().resolveFileName("test.obj");

	s3d::Scene::Pointer scene = s3d::Scene::Pointer::create();
	OBJLoader loader(modelName, OBJLoader::Option_CalculateTangents);
	auto model = loader.load(rc, scene->storage(), localCache);
	model->setParent(scene.ptr());

	Camera cam;
	cam.lookAt(vec3(-70.0f, 70.0f, 70.0f));
	cam.perspectiveProjection(vector2ToFloat(textureSize).aspect(), DEG_60, 1.0f, 1024.0f);

	_rt.setOutputMethod([this](const vec2i& pixel, const vec4& color)
	{
		DataStorage<vec4> vec4data(reinterpret_cast<vec4*>(_textureData.binary()), _textureData.dataSize());
		vec4data[pixel.x + pixel.y * _texture->size().x] = color;
	});

	_rt.perform(scene, cam, _texture->size());
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
