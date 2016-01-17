#include <et/models/objloader.h>
#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include <et/json/json.h>
#include <et/core/conversion.h>
#include <et/imaging/textureloader.h>
#include "maincontroller.hpp"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
	p.windowStyle |= WindowStyle_Sizable;
	p.windowSize = WindowSize::Predefined;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality::None;
	p.contextBaseSize = vec2i(1024, 640);
	p.contextSize = p.contextBaseSize;
    p.enableHighResolutionContext = true;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_rc = rc;
	srand(static_cast<unsigned int>(time(nullptr)));
	
	ObjectsCache localCache;

#if (ET_PLATFORM_WIN)
	application().pushSearchPath("..");
	application().pushSearchPath("..\\..");
	application().pushSearchPath("..\\..\\..");
	application().pushSearchPath("..\\..\\..\\..");
	application().pushSearchPath("Q:\\SDK\\Models");
	application().pushSearchPath("Q:\\SDK\\Textures");
#elif (ET_PLATFORM_MAC)
	application().pushSearchPath("/Volumes/Development/SDK/Models");
	application().pushSearchPath("/Volumes/Development/SDK/Textures");
#endif
	
	auto configName = application().resolveFileName("config/config.json");
	
	ValueClass vc = ValueClass_Invalid;
	_options = json::deserialize(loadTextFile(configName), vc);
	ET_ASSERT(vc == ValueClass_Dictionary);
	
	auto modelName = application().resolveFileName(_options.stringForKey("model-name")->content);
	
	_scene = s3d::Scene::Pointer::create();
	OBJLoader loader(modelName, OBJLoader::Option_CalculateTangents);
	auto model = loader.load(rc, this, _scene->storage(), localCache);
	model->setParent(_scene.ptr());
	
	Raytrace::Options rtOptions;
	rtOptions.raysPerPixel = static_cast<size_t>(_options.integerForKey("rays-per-pixel", 32)->content);
	rtOptions.maxKDTreeDepth = static_cast<size_t>(_options.integerForKey("kd-tree-max-depth", 4)->content);
	rtOptions.renderRegionSize = static_cast<size_t>(_options.integerForKey("render-region-size", 32)->content);
	rtOptions.debugRendering = _options.integerForKey("debug-rendering", 0ll)->content != 0;
	rtOptions.renderKDTree = _options.integerForKey("render-kd-tree", 0ll)->content != 0;
	rtOptions.kdTreeSplits = static_cast<int>(_options.integerForKey("kd-tree-splits", 4)->content);
	_rt.setOptions(rtOptions);
	
	_rt.setOutputMethod([this](const vec2i& pixel, const vec4& color)
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
	
	auto textureName = application().resolveFileName("background.hdr");
	auto tex = loadTexture(textureName);
	_rt.setEnvironmentSampler(rt::EnvironmentEquirectangularMapSampler::Pointer::create(tex, rt::float4(1.0f)));
	
	_rt.renderFinished.connect([this]()
	{

	});
	
	Input::instance().keyPressed.connect([this](size_t key)
	{
		if (key == ET_KEY_SPACE)
			start();
	});
	
	_gestures.click.connect([this](const PointerInputInfo& p)
	{
		vec2i pixel(int(p.pos.x), int(p.pos.y));
		vec4 color = _rt.performAtPoint(_scene, _camera, _texture->size(), pixel);
		_rt.output(vec2i(pixel.x, _texture->size().y - pixel.y), color);
	});
	
	Input::instance().keyPressed.invokeInMainRunLoop(ET_KEY_SPACE);
}

void MainController::start()
{
	_rt.stop();
	
	vec2i textureSize = _rc->size();
	
	_textureData.resize(textureSize.square() * sizeof(vec4));
	_textureData.fill(0);
	
	BinaryDataStorage proxy(reinterpret_cast<unsigned char*>(_textureData.data()), _textureData.dataSize());
	_texture = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA32F,
		textureSize, TextureFormat::RGBA, DataFormat::Float, proxy, "output-texture");
	
	const vec3 lookPoint = arrayToVec3(_options.arrayForKey("camera-view-point"));
	const vec3 offset = arrayToVec3(_options.arrayForKey("camera-offset"));
	float cameraFOV = _options.floatForKey("camera-fov", 60.0f)->content * TO_RADIANS;
	float cameraPhi = _options.floatForKey("camera-phi", 0.0f)->content * TO_RADIANS;
	float cameraTheta = _options.floatForKey("camera-theta", 0.0f)->content * TO_RADIANS;
	float cameraDistance = _options.floatForKey("camera-distance", 3.0f)->content;
	_camera.perspectiveProjection(cameraFOV, vector2ToFloat(textureSize).aspect(), 0.1f, 2048.0f);
	_camera.lookAt(cameraDistance * fromSpherical(cameraTheta, cameraPhi) + offset, lookPoint);
	
	_rt.perform(_scene, _camera, _texture->size());
}

void MainController::applicationWillTerminate()
{
	_rt.stop();
}

void MainController::render(et::RenderContext* rc)
{
	rc->renderer()->clear(true, true);
	
	if (_texture.valid())
	{
		_texture->updateDataDirectly(rc, _texture->size(),
			_textureData.binary(), _textureData.dataSize());
		rc->renderer()->renderFullscreenTexture(_texture);
	}
}

Material::Pointer MainController::materialWithName(const std::string&)
{
	return Material::Pointer();
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return et::ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "RT demo"); }
