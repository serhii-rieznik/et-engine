#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/primitives.h>
#include "DemoSceneRenderer.h"

using namespace demo;

void SceneRenderer::init(et::RenderContext* rc)
{
    _rc = rc;
}

void SceneRenderer::setScene(et::s3d::Scene::Pointer aScene)
{
	_scene = aScene;
}

void SceneRenderer::render(const et::Camera& cam, const et::Camera& observer)
{
    _sceneRenderer.render(_rc, _scene.reference(), cam);
}
