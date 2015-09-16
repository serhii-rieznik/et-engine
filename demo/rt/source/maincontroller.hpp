#pragma once

#include <et/app/application.h>
#include <et/rt/raytrace.h>
#include <et/input/gestures.h>

namespace demo
{
	class MainController : public et::IApplicationDelegate
	{
		et::ApplicationIdentifier applicationIdentifier() const;
		void setApplicationParameters(et::ApplicationParameters&);
		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void render(et::RenderContext*);
		void applicationWillTerminate();
        
        void start();

	private:
        et::Dictionary _options;
        et::RenderContext* _rc = nullptr;
		et::Raytrace _rt;
		et::Texture::Pointer _texture;
		et::DataStorage<et::vec4> _textureData;
		et::GesturesRecognizer _gestures;
		et::Camera _camera;
		et::s3d::Scene::Pointer _scene;
	};
}
