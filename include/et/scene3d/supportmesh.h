/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
			SupportMesh(const std::string& name, const VertexArrayObject& ib, const Material& material,
				IndexType startIndex, size_t numIndexes, Element* parent = 0);

			ElementType type() const 
				{ return ElementType_SupportMesh; }

			float radius() const
				{ return _radius; }

			const CollisionData& triangles() const
				{ return _data; }

			void setNumIndexes(size_t num);
			void fillCollisionData(VertexArray::Pointer v, IndexArray::Pointer i);

			SupportMesh* duplicate();

			Sphere sphere();
			AABB aabb();
			OBB obb();

			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);

			mat4 finalTransform();
			mat4 finalTransformInverse();
			float finalTransformScale();

		private:
			void buildInverseTransform();

		private:
			mat4 _cachedInverseTransform;
			CollisionData _data;
			vec3 _size;
			vec3 _center;
			float _radius;
			float _cachedFinalTransformScale;
			bool _inverseTransformValid;
		};
	}
}