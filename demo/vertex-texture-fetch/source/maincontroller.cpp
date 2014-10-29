#include <et/models/objloader.h>
#include <et/primitives/primitives.h>
#include "maincontroller.h"

using namespace et;
using namespace demo;

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.supportedInterfaceOrientations =
		InterfaceOrientation_Any & (~InterfaceOrientation_PortraitUpsideDown);

#if (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
	p.contextBaseSize = vec2i(800, 600);
	p.contextSize = p.contextBaseSize;
#endif
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	rc->renderState().setClearColor(vec4(0.25f));
	
	_sample.prepare(rc);

	ET_CONNECT_EVENT(_gestures.pressed, MainController::pointerPressed)
	
	ET_CONNECT_EVENT(input().charactersEntered, MainController::onCharactersEntered)
	ET_CONNECT_EVENT(_gestures.drag, MainController::onDrag)
	ET_CONNECT_EVENT(_gestures.zoom, MainController::onZoom)
	ET_CONNECT_EVENT(_gestures.scroll, MainController::onScroll)
	
	ObjectsCache localCache;
	OBJLoader loader(rc, "data/models/bunny.obj");
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	s3d::SupportMesh::Pointer mesh = container->childrenOfType(s3d::ElementType_SupportMesh).front();
	
	if (mesh.valid())
	{
		const auto& tris = mesh->triangles();
		
		VertexDeclaration decl(true, Usage_Position, Type_Vec3);
		decl.push_back(Usage_Normal, Type_Vec3);
		
		VertexArray::Pointer vdata = VertexArray::Pointer::create(decl, 3 * tris.size());
		
		size_t i = 0;
		auto verts = vdata->chunk(Usage_Position).accessData<vec3>(0);
		for (const auto& t : tris)
		{
			verts[i++] = t.v3();
			verts[i++] = t.v2();
			verts[i++] = t.v1();
		}
		IndexArray::Pointer idata = IndexArray::Pointer::create(IndexArrayFormat_32bit, 0, PrimitiveType_Triangles);
		vdata = primitives::buildLinearIndexArray(vdata, idata);
		primitives::calculateNormals(vdata, idata, 0, idata->primitivesCount());
		
		_sample.setModelToDraw(rc->vertexBufferFactory().createVertexArrayObject("model", vdata, BufferDrawType_Static,
			idata, BufferDrawType_Static));
	}
}

void MainController::applicationWillTerminate()
{
	
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
}

void MainController::applicationWillActivate()
{

}

void MainController::applicationWillDeactivate()
{
	
}

void MainController::render(et::RenderContext* rc)
{
	rc->renderer()->clear();
	_sample.render(rc);
}

void MainController::idle(float)
{
	
}

void MainController::pointerPressed(et::vec2, et::PointerType)
{
	_sample.stopCamera();
}

void MainController::onDrag(vec2 p, PointerType t)
{
#if (!ET_PLATFORM_IOS)
	if (t == PointerType_General)
		_sample.panCamera(5.0f * p);
	else
#endif
		_sample.dragCamera(p);
}

void MainController::onScroll(vec2 p, PointerOrigin o)
{
	if (o == PointerOrigin_Mouse)
		_sample.zoom(1.0f + 0.25f * p.y);
	else
		_sample.panCamera(300.0f * vec2(p.x, -p.y));
}

void MainController::onZoom(float v)
{
	_sample.zoom(v);
}

void MainController::onCharactersEntered(std::string chars)
{
	int upCase = ::toupper(static_cast<int>(chars.front() & 0xffffffff));

	if (upCase == 'O')
		_sample.toggleObserving();

	if (upCase == 'W')
		_sample.toggleWireframe();
}

ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "Test"); }

IApplicationDelegate* Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }
