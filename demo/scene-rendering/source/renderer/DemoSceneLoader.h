#pragma once

#include <et/scene3d/scene3d.h>

namespace demo
{
	class SceneLoader
	{
	public:
		void init(et::RenderContext*);
		
		et::s3d::Scene::Pointer loadFromFile(const std::string&);
		
	private:
		void loadObjFile(const std::string&, et::s3d::Scene::Pointer);
		
	private:
		et::RenderContext* _rc = nullptr;
	};
}
