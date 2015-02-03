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

#include "raytracer/RaytraceScene.h"
#include "raytracer/RaytraceThread.h"

namespace rt
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler, public RaytraceThreadDelegate
	{
		et::ApplicationIdentifier applicationIdentifier() const;
		void setRenderContextParameters(et::RenderContextParameters&);
		
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		void applicationWillTerminate();
		
		void render(et::RenderContext*);
		void updateTitle();
		void onKeyPressed(size_t);
		void renderFinished(uint64_t);
		void startRender(bool);
		
	private:
		et::vec2i imageSize();
		const RaytraceScene& scene();
		OutputFunction outputFunction();
		
		bool fetchNewRenderRect(et::vec2i& origin, et::vec2i& size, bool& preview);
		
		void updateCamera();
		
	private:
		et::GesturesRecognizer _gestures;
		et::InterpolationValue<et::vec2> _cameraAngles;
						
		et::Texture::Pointer _result;
		et::DataStorage<et::vec4ub> _textureData;
		
		RaytraceScene _scene;
		std::vector<RaytraceThread*, et::SharedBlockAllocatorSTDProxy<RaytraceThread*>> _threads;
		
		RenderRects _renderRects;
		OutputFunction _outputFunction;
		et::CriticalSection _csLock;
		
		float _startTime = 0.0f;

		uint64_t _estimatedTime = 0;
		uint64_t _maxElapsedTime = 0;

		size_t _previewSamples = 4;
		size_t _previewBounces = 1;
		
		size_t _productionSamples = 128;
		size_t _productionBounces = 64;
		
		size_t _renderRectIndex = 0;
		
		bool _shouldRender = true;
		bool _interactiveRendering = false;
		bool _shouldUpdateTexture = true;
		bool _rendering = false;
	};
}