#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>
#include <et/camera/cameramovingcontroller.h>
#include <et/scene3d/scene3drenderer.h>
#include "renderer/DemoSceneLoader.h"

namespace demo
{
	class MainController : public et::IApplicationDelegate
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;
		
		void setApplicationParameters(et::ApplicationParameters&);
		void setRenderContextParameters(et::RenderContextParameters&);
		
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		
		void render(et::RenderContext*);
		
	private:
        et::Camera _camera;
        et::CameraMovingController::Pointer _cameraController;
		et::s3d::Scene::Pointer _scene;
		et::s3d::Renderer _renderer;
		demo::SceneLoader _loader;
	};
}
