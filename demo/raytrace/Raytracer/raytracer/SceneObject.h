//
//  SceneObject.h
//  Raytracer
//
//  Created by Sergey Reznik on 16/11/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include "SceneIntersection.h"

namespace rt
{
	class SceneObject
	{
	public:
		virtual ~SceneObject() { }
		virtual bool intersects(const et::ray3d&, et::vec3&, et::vec3&) = 0;
	
	public:
		SceneObject(size_t m) :
			_materialId(m) { }
		
		size_t materialId() const
			{ return _materialId; }

	private:
		size_t _materialId = MissingObjectIndex;
	};
	
	class DummyObject : public SceneObject
	{
	public:
		DummyObject() :
			SceneObject(MissingObjectIndex) { }
		
		bool intersects(const et::ray3d&, et::vec3&, et::vec3&)
			{ return false; }
	};
	
	class MeshObject : public SceneObject
	{
	public:
		MeshObject(const et::s3d::SupportMesh::Pointer& m, size_t mat);
		
		bool intersects(const et::ray3d&, et::vec3&, et::vec3&);
		
	private:
		et::s3d::SupportMesh::CollisionData _triangles;
		et::Sphere _boundingSphere;
		et::AABB _boundingBox;
	};
}
