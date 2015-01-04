/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/mesh.h>
#include <et/collision/collision.h>

namespace et
{
	namespace s3d
	{
		class SupportMesh : public Mesh
		{
		public:
			ET_DECLARE_POINTER(SupportMesh)
			typedef DataStorage<triangle> CollisionData;

		public:
			SupportMesh(const std::string& name = defaultMeshName, Element* parent = 0);
			
			SupportMesh(const std::string& name, const VertexArrayObject& ib, const Material::Pointer& material,
				uint32_t startIndex, size_t numIndexes, Element* parent = 0);

			ElementType type() const 
				{ return ElementType_SupportMesh; }

			float radius() const
				{ return _radius; }

			const vec3& center() const
				{ return _center; }

			const CollisionData& triangles() const
				{ return _data; }

			void setNumIndexes(size_t num);
			void fillCollisionData(const VertexArray::Pointer& v, const IndexArray::Pointer& i);

			SupportMesh* duplicate();

			Sphere sphere();
			const AABB& aabb();
			OBB obb();
			
			const vec3& size() const
				{ return _size; }
			
			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);
			
			float finalTransformScale();

		private:
			void transformInvalidated() override;
			void buildInverseTransform();

		private:
			CollisionData _data;
			AABB _cachedAABB;
			
			vec3 _size;
			vec3 _center;
			float _radius = 0.0f;
			bool _shouldBuildAABB = true;
		};
	}
}
