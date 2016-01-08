/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/renderableelement.h>
#include <et/scene3d/meshdeformer.h>

namespace et
{
	namespace s3d
	{
		class Mesh : public RenderableElement
		{
		public:
			ET_DECLARE_POINTER(Mesh)
			
			static const std::string defaultMeshName;

		public:
			Mesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
			Mesh(const std::string&, const SceneMaterial::Pointer&, BaseElement* = nullptr);
			
			ElementType type() const override
				{ return ElementType::Mesh; }

			Mesh* duplicate() override;
			
			void serialize(Dictionary, const std::string&) override;
			void deserialize(Dictionary, SerializationHelper*) override;

			void calculateSupportData();
			
			const Sphere& boundingSphere();
			const Sphere& boundingSphereUntransformed();
			
			const BoundingBox& tranformedBoundingBox();
			
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
			
			struct SupportData
			{
				Sphere untranfromedBoundingSphere;
				Sphere tranfromedBoundingSphere;
				BoundingBox transformedBoundingBox;
				bool shouldUpdateBoundingBox = true;
				bool shouldUpdateBoundingSphere = true;
				bool shouldUpdateBoundingSphereUntransformed = true;
			};

		private:
			MeshDeformer::Pointer _deformer;
			SupportData _supportData;
			BoundingBox _boundingBox;
			float _boundingSphereRadius = 0.0f;
			std::vector<mat4> _undeformedTransformationMatrices;
		};
	}
}
