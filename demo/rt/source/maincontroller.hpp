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

	private:
		et::Raytrace _rt;
		et::Texture::Pointer _texture;
		et::BinaryDataStorage _textureData;
		et::GesturesRecognizer _gestures;
		et::Camera _camera;
		et::s3d::Scene::Pointer _scene;
	};
}
