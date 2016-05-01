#include <et/rendering/primitives.h>
#include <et/scene3d/objloader.h>
#include <et/app/application.h>

#include "DemoSceneLoader.h"

using namespace et;
using namespace demo;

void SceneLoader::init(et::RenderContext* rc)
{
	_rc = rc;
    _defaultMaterial = rc->materialFactory().loadMaterial(application().resolveFileName("media/materials/microfacet.material"));
}

et::s3d::Scene::Pointer SceneLoader::loadFromFile(const std::string& fileName)
{
    ET_ASSERT(_rc);
	
	et::s3d::Scene::Pointer result = et::s3d::Scene::Pointer::create();
	
	auto ext = getFileExt(fileName);
	lowercase(ext);
	
	if (ext == "obj")
	{
		loadObjFile(fileName, result);
	}
	else
	{
		ET_FAIL("Not implemented")
	}
	
	return result;
}

void SceneLoader::loadObjFile(const std::string& fileName, et::s3d::Scene::Pointer scene)
{
	ObjectsCache localCache;
    OBJLoader loader(fileName, OBJLoader::Option_JustLoad);
    auto container = loader.load(_rc, this, scene->storage(), localCache);

    // compute bounding box
    vec3 minVertex(std::numeric_limits<float>::max());
    vec3 maxVertex(-std::numeric_limits<float>::max());
    auto meshes = container->childrenOfType(et::s3d::ElementType::Mesh);
    for (s3d::Mesh::Pointer mesh : meshes)
    {
        minVertex = minv(minVertex, mesh->tranformedBoundingBox().minVertex());
        maxVertex = maxv(maxVertex, mesh->tranformedBoundingBox().maxVertex());
    }
    vec3 bboxCenter = 0.5f * (minVertex + maxVertex);
 
    // move objects to scene
	auto allObjects = container->children();
	for (auto c : allObjects)
		c->setParent(scene.ptr());
    
    // add light
    s3d::Light::Pointer lp = s3d::Light::Pointer::create();
    lp->camera().setPosition(vec3(bboxCenter.x, maxVertex.y + std::abs(maxVertex.y), bboxCenter.z));
    lp->setParent(scene.ptr());
}

et::Material::Pointer SceneLoader::materialWithName(const std::string& key)
{
    auto i = _materialMap.find(key);
    if (i == _materialMap.end())
    {
        log::error("No material for key: %s", key.c_str());
        return _defaultMaterial;
    }
    
    return i->second;
}
