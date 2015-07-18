#pragma once

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/camera/camera.h>
#include <et/timers/intervaltimer.h>

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

		void createModels(et::RenderContext*);
		void createTextures(et::RenderContext*);
		void createPrograms(et::RenderContext*);

	private:
		et::Camera _camera;
		et::Texture::Pointer _noiseTexture;
		et::VertexArrayObject _testModel;
		et::Program::Pointer _defaultProgram;
		et::IntervalTimer _frameTimeTimer;
		et::mat4 _transformMatrix;
	};
}
