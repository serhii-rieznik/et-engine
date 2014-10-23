#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>
#include "sample/sample.h"

namespace demo
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setRenderContextParameters(et::RenderContextParameters&);

		void applicationDidLoad(et::RenderContext*);
		void applicationWillTerminate();

		void applicationWillResizeContext(const et::vec2i&);
		
		void applicationWillActivate();
		void applicationWillDeactivate();

		void render(et::RenderContext*);
		void idle(float);

		void onDrag(et::vec2, et::PointerType);
		void onScroll(et::vec2, et::PointerOrigin);
		void onZoom(float);
		void onCharactersEntered(std::string chars);
		
		void pointerPressed(et::vec2, et::PointerType);
		
	private:
		et::ObjectsCache _mainTextureCache;
		et::GesturesRecognizer _gestures;
		et::VertexArrayObject _sphere;
		Sample _sample;
	};
}
