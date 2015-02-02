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
			SupportMesh(const std::string& = defaultMeshName, Element* = nullptr);
			
			SupportMesh(const std::string&, const VertexArrayObject&, const Material::Pointer&,
				uint32_t, uint32_t, const VertexStorage::Pointer&, const IndexArray::Pointer&, Element* = nullptr);

			ElementType type() const 
				{ return ElementType_SupportMesh; }

			float radius() const
				{ return _radius; }

			const CollisionData& triangles() const
				{ return _data; }

			void setNumIndexes(uint32_t num);
			void fillCollisionData(const VertexArray::Pointer& v, const IndexArray::Pointer& i);
			void fillCollisionData(const VertexStorage::Pointer& v, const IndexArray::Pointer& i);

			SupportMesh* duplicate();

			Sphere sphere();
			const AABB& aabb();
			OBB obb();
			
			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);
			
			float finalTransformScale();

		private:
			void transformInvalidated() override;
			void buildInverseTransform();

		private:
			CollisionData _data;
			AABB _cachedAABB;
			float _radius = 0.0f;
			bool _shouldBuildAABB = true;
		};
	}
}
