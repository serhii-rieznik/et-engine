//
//  DemoSceneRenderer.h
//  SceneRendering
//
//  Created by Sergey Reznik on 14/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/scene3d/scene3d.h>

namespace demo
{
	class SceneRenderer
	{
	public:
		void init(et::RenderContext*);
		void render(const et::Camera&);
		
		void setScene(et::s3d::Scene::Pointer);
		
	private:
		struct
		{
			et::Program::Pointer basic;
		} programs;
		
	private:
		et::RenderContext* _rc = nullptr;
		et::s3d::Scene::Pointer _scene;
		et::s3d::Element::List _allObjects;
		
		et::Texture _defaultTexture;
		et::Texture _defaultNormalTexture;
	};
}
