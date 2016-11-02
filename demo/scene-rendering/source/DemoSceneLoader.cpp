#include <et/rendering/base/primitives.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/objloader.h>
#include <et/app/application.h>

#include "DemoSceneLoader.h"

using namespace et;
using namespace demo;

void SceneLoader::init(et::RenderContext* rc)
{
	_rc = rc;
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
    OBJLoader loader(fileName, OBJLoader::Option_CalculateTransforms);
    auto container = loader.load(_rc->renderer(), scene->storage(), localCache);

    // compute bounding box
    vec3 minVertex(std::numeric_limits<float>::max());
    vec3 maxVertex(-std::numeric_limits<float>::max());
    auto meshes = container->childrenOfType(et::s3d::ElementType::Mesh);
    for (s3d::Mesh::Pointer mesh : meshes)
    {
        minVertex = minv(minVertex, mesh->tranformedBoundingBox().minVertex());
        maxVertex = maxv(maxVertex, mesh->tranformedBoundingBox().maxVertex());
    }

    // move objects to scene
	auto allObjects = container->children();
	for (auto c : allObjects)
		c->setParent(scene.pointer());
    
    // add light
    s3d::Light::Pointer lp = s3d::Light::Pointer::create();
	vec3 bboxCenter = 0.5f * (minVertex + maxVertex);
    lp->camera()->setPosition(vec3(bboxCenter.x, maxVertex.y + 2.0f * std::abs(maxVertex.y), bboxCenter.z));
    lp->setParent(scene.pointer());
}
