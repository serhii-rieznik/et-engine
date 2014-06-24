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
	_resourceManager.load(rc);
	_gui = gui::Gui::Pointer(new gui::Gui(rc));

	_mainMenu = MainMenuLayout::Pointer(new MainMenuLayout(rc, _resourceManager));
	_gui->pushLayout(_mainMenu);

	_sample.prepare(rc);

	ET_CONNECT_EVENT(_gestures.pressed, MainController::pointerPressed)
	
	ET_CONNECT_EVENT(input().keyPressed, MainController::onKeyPressed)
	ET_CONNECT_EVENT(_gestures.drag, MainController::onDrag)
	ET_CONNECT_EVENT(_gestures.zoom, MainController::onZoom)
	ET_CONNECT_EVENT(_gestures.scroll, MainController::onScroll)
}

void MainController::applicationWillTerminate()
{
	
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	vec2 fsz(static_cast<float>(sz.x), static_cast<float>(sz.y));
	_gui->layout(fsz);
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
	_gui->render(rc);
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

void MainController::onKeyPressed(size_t key)
{
	int upCase = ::toupper(static_cast<int>(key & 0xffffffff));

	if (upCase == 'O')
		_sample.toggleObserving();

	if (upCase == 'W')
		_sample.toggleWireframe();
}

ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "Test"); }

IApplicationDelegate* Application::initApplicationDelegate()
	{ return new MainController; }
