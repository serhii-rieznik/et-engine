#pragma once

#include <et/scene3d/scene3d.h>
#include <et/scene3d/scene3drenderer.h>

namespace demo
{
	class SceneRenderer
	{
	public:
		void init(et::RenderContext*);
		void render(const et::Camera&, const et::Camera&);
		
		void setScene(et::s3d::Scene::Pointer);
				
	private:
		et::RenderContext* _rc = nullptr;
		et::s3d::Scene::Pointer _scene;
        et::s3d::Renderer _sceneRenderer;
	};
}
