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
			SupportMesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
			
			SupportMesh(const std::string&, const VertexArrayObject&, const Material::Pointer&,
				uint32_t, uint32_t, const VertexStorage::Pointer&, const IndexArray::Pointer&, BaseElement* = nullptr);

			ElementType type() const 
				{ return ElementType::SupportMesh; }

			const CollisionData& triangles() const
				{ return _data; }

			void setNumIndexes(uint32_t num);
			void fillCollisionData(const VertexStorage::Pointer& v, const IndexArray::Pointer& i);

			SupportMesh* duplicate();

			OBB obb();
			
			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, SerializationHelper*);
			

		private:
			void transformInvalidated() override;
			void buildInverseTransform();

		private:
			CollisionData _data;
		};
	}
}
