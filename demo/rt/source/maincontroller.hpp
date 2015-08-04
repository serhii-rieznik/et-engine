#pragma once

#include <et/app/application.h>
#include <et/rt/raytrace.h>

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
	};
}
