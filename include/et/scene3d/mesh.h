/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vertexarrayobject.h>
#include <et/scene3d/renderableelement.h>
#include <et/scene3d/meshdeformer.h>
#include <et/collision/aabb.h>
#include <et/collision/sphere.h>
#include <et/collision/obb.h>

namespace et
{
	namespace s3d
	{
		class Mesh : public RenderableElement
		{
		public:
			ET_DECLARE_POINTER(Mesh)
			
			static const std::string defaultMeshName;
			
			struct SupportData
			{
				vec3 minMaxCenter;
				vec3 averageCenter;
				vec3 dimensions;
				float boundingSphereRadius = 0.0f;
				bool valid = false;
			};

		public:
			Mesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
			
			Mesh(const std::string&, const VertexArrayObject&, const Material::Pointer&,
				uint32_t, uint32_t, BaseElement* = nullptr);
			
			Mesh(const std::string&, const VertexArrayObject&, const Material::Pointer&,
				uint32_t, uint32_t, const VertexStorage::Pointer&, const IndexArray::Pointer&, 
				BaseElement* = nullptr);

			ElementType type() const override
				{ return ElementType::Mesh; }

			Mesh* duplicate() override;

			VertexArrayObject& vertexArrayObject();
			const VertexArrayObject& vertexArrayObject() const;

			VertexBuffer::Pointer& vertexBuffer();
			const VertexBuffer::Pointer& vertexBuffer() const;

			IndexBuffer::Pointer& indexBuffer();
			const IndexBuffer::Pointer& indexBuffer() const;

			uint32_t startIndex() const;
			void setStartIndex(uint32_t index);
			
			uint32_t numIndexes() const;
			virtual void setNumIndexes(uint32_t num);

			void setVertexBuffer(VertexBuffer::Pointer);
			void setIndexBuffer(IndexBuffer::Pointer);
			void setVertexArrayObject(VertexArrayObject);

			void serialize(Dictionary, const std::string&) override;
			void deserialize(Dictionary, SerializationHelper*) override;

			void setVertexStorage(VertexStorage::Pointer);
			void setIndexArray(IndexArray::Pointer);
			
			void cleanupLodChildren();
			void attachLod(uint32_t level, Mesh::Pointer mesh);

			void setLod(uint32_t level);
						
			void calculateSupportData();
			
			const SupportData& supportData() const
				{ return _supportData; }

			const VertexStorage::Pointer& vertexStorage() const
				{ return _vertexStorage; }
			
			const IndexArray::Pointer& indexArray() const
				{ return _indexArray; }
			
			const Sphere& boundingSphere();
			const AABB& boundingBox();
			const OBB& orientedBoundingBox();
			
			float finalTransformScale();
			
			MeshDeformer::Pointer deformer()
				{ return _deformer; }
			
			void setDeformer(MeshDeformer::Pointer d)
				{ _deformer = d; }
			
			const std::vector<mat4>& deformationMatrices();
			
			VertexStorage::Pointer bakeDeformations();
			
		private:
			Mesh* currentLod();
			const Mesh* currentLod() const;
			void transformInvalidated() override;

		private:
			VertexArrayObject _vao;
			VertexStorage::Pointer _vertexStorage;
			IndexArray::Pointer _indexArray;
			
			AABB _cachedBoundingBox;
			OBB _cachedOrientedBoundingBox;
			Sphere _cachedBoundingSphere;
			
			SupportData _supportData;
			std::map<uint32_t, Mesh::Pointer> _lods;
			std::vector<mat4> _undeformedTransformationMatrices;
			MeshDeformer::Pointer _deformer;
			
			uint32_t _startIndex = 0;
			uint32_t _numIndexes = 0;
			uint32_t _selectedLod = 0;
			
			bool _shouldUpdateBoundingBox = true;
			bool _shouldUpdateOrientedBoundingBox = true;
			bool _shouldUpdateBoundingSphere = true;
		};
	}
}
