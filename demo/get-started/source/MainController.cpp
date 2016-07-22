#include "MainController.h"
#include <et/rendering/primitives.h>

void demo::MainController::setApplicationParameters(et::ApplicationParameters& p)
{
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

	et::VertexArray::Pointer vertices = et::VertexArray::Pointer::create(decl, 0);
	et::primitives::createIcosahedron(vertices, 5.0f);
	et::primitives::tesselateTriangles(vertices);

	et::IndexArray::Pointer indices = et::IndexArray::Pointer::create(et::IndexArrayFormat::Format_16bit,
		vertices->size(), et::PrimitiveType::Triangles);
	indices->linearize(vertices->size());

	_testModel = rc->vertexBufferFactory().createVertexArrayObject("test-model",
		vertices, et::BufferDrawType::Static, indices, et::BufferDrawType::Static);

	_transformMatrix = et::identityMatrix;
}

void demo::MainController::loadProgram(et::RenderContext* rc)
{
	auto materialFile = et::application().resolveFileName("media/materials/normals.material");
	_defaultMaterial = rc->materialFactory().loadMaterial(materialFile);
}

 void demo::MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	et::vec2 floatSize = vector2ToFloat(sz);
	_camera.perspectiveProjection(QUARTER_PI, floatSize.aspect(), 1.0f, 100.0f);
}

void demo::MainController::render(et::RenderContext* rc)
{
	_transformMatrix *= rotationYXZMatrix(et::vec3(2.0f, 0.5f, -1.0f) * _frameTimeTimer.lap());

	et::RenderPass::ConstructionInfo passInfo;
	passInfo.camera = _camera;
	passInfo.colorAttachment.loadOperation = et::FramebufferOperation::Clear;
	passInfo.colorAttachment.clearColor = et::vec4(0.1f, 0.2f, 0.3f, 1.0f);
	passInfo.depthAttachment.loadOperation = et::FramebufferOperation::Clear;
	passInfo.depthAttachment.clearDepth = 1.0f;

	et::RenderPass::Pointer pass = rc->renderer()->allocateRenderPass(passInfo);

	et::RenderBatch::Pointer batch = et::RenderBatch::Pointer::create(_defaultMaterial, _testModel, _transformMatrix);
	pass->pushRenderBatch(batch);
	
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
