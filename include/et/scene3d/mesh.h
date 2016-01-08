/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

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
				
				AABB cachedBoundingBox;
				OBB cachedOrientedBoundingBox;
				Sphere cachedBoundingSphere;
				Sphere cachedBoundingSphereUntransformed;
				
				float boundingSphereRadius = 0.0f;
				
				bool shouldUpdateBoundingBox = true;
				bool shouldUpdateOrientedBoundingBox = true;
				bool shouldUpdateBoundingSphere = true;
				bool shouldUpdateBoundingSphereUntransformed = true;
				bool valid = false;
			};

		public:
			Mesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
			Mesh(const std::string&, const SceneMaterial::Pointer&, BaseElement* = nullptr);
			
			ElementType type() const override
				{ return ElementType::Mesh; }

			Mesh* duplicate() override;
			
			void serialize(Dictionary, const std::string&) override;
			void deserialize(Dictionary, SerializationHelper*) override;

			void calculateSupportData();
			
			const SupportData& supportData() const
				{ return _supportData; }
			
			const Sphere& boundingSphere();
			const Sphere& boundingSphereUntransformed();
			const AABB& boundingBox();
			const OBB& orientedBoundingBox();
			
			float finalTransformScale();
			
			MeshDeformer::Pointer deformer()
				{ return _deformer; }
			
			void setDeformer(MeshDeformer::Pointer d)
				{ _deformer = d; }
			
			const std::vector<mat4>& deformationMatrices();

			bool skinned() const;
			VertexStorage::Pointer bakeDeformations();
			
		protected:
			void duplicateMeshPropertiesToMesh(s3d::Mesh*);
			void transformInvalidated() override;

		private:
			MeshDeformer::Pointer _deformer;
			SupportData _supportData;
			std::vector<mat4> _undeformedTransformationMatrices;
			uint32_t _selectedLod = 0;
		};
	}
}
