#pragma once

#include <et/scene3d/scene3d.h>

namespace demo
{
    class SceneLoader : public et::MaterialProvider
	{
	public:
		void init(et::RenderContext*);
		
		et::s3d::Scene::Pointer loadFromFile(const std::string&);
		
	private:
		void loadObjFile(const std::string&, et::s3d::Scene::Pointer);
        et::Material::Pointer materialWithName(const std::string&);
		
	private:
		et::RenderContext* _rc = nullptr;
        et::Material::Pointer _defaultMaterial;
        et::UnorderedMap<std::string, et::Material::Pointer> _materialMap;
	};
}
