#include <et/models/objloader.h>
#include <et/primitives/primitives.h>
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
	ET_ASSERT(_rc)
	
	et::s3d::Scene::Pointer result = et::s3d::Scene::Pointer::create();
	
	auto ext = getFileExt(fileName);
	lowercase(ext);
	
	if (ext == "obj")
	{
		loadObjFile(fileName, result);
	}
	else if (ext == "etm")
	{
		ET_FAIL("Not implemented")
	}
	else
	{
		ET_FAIL("Not implemented")
	}

/*	
	auto storages = result->childrenOfType(s3d::ElementType_Storage);
	auto meshes = result->childrenOfType(s3d::ElementType_SupportMesh);
	
	for (s3d::Scene3dStorage::Pointer storage : storages)
	{
		for (auto va : storage->vertexArrays())
		{
			auto decl = va->decl();
			
			if (!decl.has(VertexAttributeUsage::Tangent))
			{
				decl.push_back(VertexAttributeUsage::Tangent, VertexAttributeType::Vec3);
				
				VertexArray::Pointer updated = VertexArray::Pointer::create(decl, va->size());
				
				for (auto& c : va->chunks())
					c->copyTo(updated->chunk(c->usage()).reference());
				
				primitives::calculateTangents(updated, storage->indexArray(), 0,
					static_cast<uint32_t>(storage->indexArray()->primitivesCount()));
				
				for (s3d::Mesh::Pointer mesh : meshes)
				{
					auto vao = result->vaoWithIdentifiers(mesh->vbName(), mesh->ibName());
					
					if (vao.valid())
					{
						_rc->renderState().bindVertexArray(vao);
						vao->setVertexBuffer(_rc->vertexBufferFactory().createVertexBuffer(va->name(), updated, BufferDrawType::Static));
					}
				}
				
				va.reset(updated.ptr());
				updated.reset(nullptr);
			}
		}
	}
*/	
	return result;
}

void SceneLoader::loadObjFile(const std::string& fileName, et::s3d::Scene::Pointer scene)
{
	ObjectsCache localCache;
	OBJLoader loader(_rc, fileName);
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	auto allObjects = container->children();
	for (auto c : allObjects)
		c->setParent(scene.ptr());
}

