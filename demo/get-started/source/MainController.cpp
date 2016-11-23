#include "MainController.h"
#include <et/core/hardware.h>
#include <et/rendering/base/primitives.h>

#if (ET_PLATFORM_WIN)
#	include "../windows/WorkingDirectory.hpp"
const et::RenderingAPI api = et::RenderingAPI::Vulkan;
#else
const et::RenderingAPI api = et::RenderingAPI::Metal;
#endif

void demo::MainController::setApplicationParameters(et::ApplicationParameters& p)
{
	p.renderingAPI = api;
	p.context.style = et::ContextOptions::Style::Sizable | et::ContextOptions::Style::Caption;
	p.context.size = 4 * et::currentScreen().frame.size() / 5;
}

void demo::MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	params.multisamplingQuality = et::MultisamplingQuality::Best;
}

void demo::MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	et::application().pushRelativeSearchPath(workingDirectory);
#endif

	_camera = et::Camera::Pointer::create();
	_camera->lookAt(et::vec3(20.0f));

	loadProgram(rc);
	createModels(rc);

	et::RenderPass::ConstructionInfo passInfo;
	passInfo.color[0].clearValue = et::vec4(0.3f, 0.3f, 0.4f, 1.0f);
	passInfo.color[0].enabled = true;
	passInfo.depth.enabled = true;
	passInfo.camera = _camera;

	_mainPass = rc->renderer()->allocateRenderPass(passInfo);

	applicationWillResizeContext(rc->size());
	_frameTimeTimer.run();
}

void demo::MainController::createModels(et::RenderContext* rc)
{
	et::VertexDeclaration decl(true);
	decl.push_back(et::VertexAttributeUsage::Position, et::DataType::Vec3);
	decl.push_back(et::VertexAttributeUsage::Normal, et::DataType::Vec3);

	et::VertexStorage::Pointer vertices = et::VertexStorage::Pointer::create(decl, 0);
	et::primitives::createIcosahedron(vertices, 5.0f);

	et::IndexArray::Pointer indices = et::IndexArray::Pointer::create(et::IndexArrayFormat::Format_16bit,
		vertices->capacity(), et::PrimitiveType::Triangles);
	indices->linearize(vertices->capacity());

	auto vb = rc->renderer()->createVertexBuffer("test-vb", vertices, et::Buffer::Location::Device);
	auto ib = rc->renderer()->createIndexBuffer("test-ib", indices, et::Buffer::Location::Device);

	et::VertexStream::Pointer vs = et::VertexStream::Pointer::create();
	vs->setIndexBuffer(ib, indices->format(), indices->primitiveType());
	vs->setVertexBuffer(vb, vertices->declaration());

	_mainBatch = et::RenderBatch::Pointer::create(_defaultMaterial->instance(), vs,
		_transformMatrix, 0, indices->actualSize());
}

void demo::MainController::loadProgram(et::RenderContext* rc)
{
	auto materialFile = et::application().resolveFileName("engine_data/materials/normals.json");
	_defaultMaterial = rc->renderer()->sharedMaterialLibrary().loadMaterial(materialFile);
}

void demo::MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	et::vec2 floatSize = vector2ToFloat(sz);
	_camera->perspectiveProjection(QUARTER_PI, floatSize.aspect(), 1.0f, 100.0f);
}

void demo::MainController::render(et::RenderContext* rc)
{
	_transformMatrix *= rotationYXZMatrix(et::vec3(2.0f, 0.5f, -1.0f) * _frameTimeTimer.lap());
	_mainBatch->setTransformation(_transformMatrix);

	_mainPass->begin();
	_mainPass->pushRenderBatch(_mainBatch);
	_mainPass->end();

	rc->renderer()->submitRenderPass(_mainPass);
}

et::ApplicationIdentifier demo::MainController::applicationIdentifier() const
{
	return et::ApplicationIdentifier("com.cheetek.et.getstarted", "Cheetek", "Get Started Demo");
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
{
	return sharedObjectFactory().createObject<demo::MainController>();
}
