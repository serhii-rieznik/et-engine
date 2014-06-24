//
//  MainController.h
//  Raytracer
//
//  Created by Sergey Reznik on 21/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include <et/input/input.h>
#include <et/input/gestures.h>
#include <et/app/application.h>
#include <et/timers/interpolationvalue.h>

namespace rt
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler
	{
		et::ApplicationIdentifier applicationIdentifier() const;
		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		void render(et::RenderContext*);
		void idle(float);
		
		void performRender(et::RenderContext*);
		void updateTitle();
		
		void onKeyPressed(size_t);
		void restartOnlineRendering();
		void restartOfflineRendering();
		
	private:
		et::Camera _mainCamera;
		et::GesturesRecognizer _gestures;
		et::InterpolationValue<et::vec2> _cameraAngles;
		
		et::Program::Pointer _mainProgram;
		et::Texture _noise;
		et::vec2 _initialScale;
		et::vec2 _initialOffset;
		et::vec2 _scale;
		et::vec2 _offset;
		et::vec3 _lightPosition;
		
		std::vector<et::vec4> _planes;
		std::vector<et::vec4> _planeColors;
		std::vector<et::vec4> _planeProps;

		std::vector<et::vec4> _spheres;
		std::vector<et::vec4> _sphereColors;
		std::vector<et::vec4> _sphereProps;
		
		int _bounces = 1;
		int _previewBounces = 1;
		int _productionBounces = 10;
		
		bool _shouldRender = true;
		bool _interactiveRendering = false;
	};
}