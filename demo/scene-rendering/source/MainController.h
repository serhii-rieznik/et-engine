#pragma once

#include "DemoSceneLoader.h"

#include <et/app/application.h>
#include <et/input/gestures.h>
#include <et/camera/cameramovingcontroller.h>
#include <et/scene3d/scene3drenderer.h>

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
		et::Dictionary _options;
		et::Camera::Pointer _camera;
        et::CameraMovingController::Pointer _cameraController;
		et::s3d::Scene::Pointer _scene;
		et::s3d::Renderer _renderer;
		demo::SceneLoader _loader;
		float _cameraFOV = QUARTER_PI;
	};
}
