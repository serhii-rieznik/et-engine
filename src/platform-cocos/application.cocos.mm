 /*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>

#if defined(ET_HAVE_COCOS)

#if !defined(ET_EMBEDDED_APPLICATION)
#
#	error Define ET_EMBEDDED_APPLICATION to compile
#
#endif

#include <et/opengl/opengl.h>
#include <et/platform-ios/applicationdelegate.h>
#include <et/rendering/rendercontext.h>

using namespace et;

void Application::platformInit()
{
	_env.updateDocumentsFolder(_identifier);
}

void Application::platformFinalize()
{
	sharedObjectFactory().deleteObject(_delegate);
	sharedObjectFactory().deleteObject(_renderContext);
	
	_renderContext = nullptr;
	_delegate = nullptr;
}

void Application::platformActivate()
{
}

void Application::platformDeactivate()
{
}

void Application::platformSuspend()
{
}

void Application::platformResume()
{
}

int Application::platformRun(int argc, char* argv[])
{
	loaded();
	return 0;
}

void Application::loaded()
{
	auto scale = [[UIScreen mainScreen] scale];
	CGRect winBounds = [[UIScreen mainScreen] bounds];
	
	RenderContextParameters renderContextParams;
	renderContextParams.contextSize.x = static_cast<int>(winBounds.size.width * scale);
	renderContextParams.contextSize.y = static_cast<int>(winBounds.size.height * scale);
	delegate()->setRenderContextParameters(renderContextParams);
	
	_renderContext = sharedObjectFactory().createObject<RenderContext>(renderContextParams, this);
	_renderingContextHandle = _renderContext->renderingContextHandle();
	_runLoop.updateTime(_lastQueuedTimeMSec);
	
    enterRunLoop();
}

void Application::quit(int exitCode)
{
	_running = false;
	
	setActive(false);
	terminated();
	
	sharedObjectFactory().deleteObject(_delegate);
	sharedObjectFactory().deleteObject(_renderContext);
	
	_delegate = nullptr;
	_renderContext = nullptr;
}

void Application::alert(const std::string& title, const std::string& message, AlertType)
{
}

void Application::setTitle(const std::string&)
{
	
}

void Application::requestUserAttention()
{
}

#else 
#	warning Define ET_HAVE_COCOS to compile
#endif
