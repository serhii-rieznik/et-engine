#include "MainController.h"
#include <et/rendering/base/primitives.h>

const et::RenderingAPI api = et::RenderingAPI::Metal;

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
	_camera.lookAt(et::vec3(20.0f));
	createModels(rc);
	loadProgram(rc);
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

	_testModel = rc->renderer()->createVertexArrayObject("test-model");
	auto vb = rc->renderer()->createVertexBuffer("test-vb", vertices, et::BufferDrawType::Static);
	auto ib = rc->renderer()->createIndexBuffer("test-ib", indices, et::BufferDrawType::Static);
	_testModel->setBuffers(vb, ib);

	_transformMatrix = et::identityMatrix;
}

void demo::MainController::loadProgram(et::RenderContext* rc)
{
	auto materialFile = et::application().resolveFileName("media/materials/normals.material");
	_defaultMaterial = et::Material::Pointer::create(rc->renderer().ptr());
	_defaultMaterial->loadFromJson(et::loadTextFile(materialFile), et::getFileFolder(materialFile));
}

 void demo::MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	et::vec2 floatSize = vector2ToFloat(sz);
	_camera.perspectiveProjection(QUARTER_PI, floatSize.aspect(), 1.0f, 100.0f);
}

void demo::MainController::render(et::RenderContext* rc)
{
	et::RenderPass::ConstructionInfo passInfo;
	passInfo.target.colorLoadOperation = et::FramebufferOperation::Clear;
	passInfo.target.depthLoadOperation = et::FramebufferOperation::Clear;
	passInfo.target.clearColor = et::vec4(0.1f, 0.2f, 0.3f, 1.0f);
	passInfo.target.clearDepth = 1.0f;
	passInfo.camera = _camera;

	et::RenderPass::Pointer pass = rc->renderer()->allocateRenderPass(passInfo);
	{
		_transformMatrix *= rotationYXZMatrix(et::vec3(2.0f, 0.5f, -1.0f) * _frameTimeTimer.lap());
		et::RenderBatch::Pointer batch = et::RenderBatch::Pointer::create(_defaultMaterial,
			_testModel, _transformMatrix);
		pass->pushRenderBatch(batch);
	}
	rc->renderer()->submitRenderPass(pass);
}

et::ApplicationIdentifier demo::MainController::applicationIdentifier() const
{
	return et::ApplicationIdentifier("com.cheetek.et.getstarted", "Cheetek", "Get Started Demo");
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
{
	return sharedObjectFactory().createObject<demo::MainController>();
}
