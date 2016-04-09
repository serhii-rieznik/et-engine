/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/skeletonelement.h>

namespace et
{
	namespace s3d
	{
		class Mesh;
		
		class MeshDeformerCluster : public AtomicCounter
		{
		public:
			ET_DECLARE_POINTER(MeshDeformerCluster)

			struct VertexWeight
			{
				uint32_t index = 0;
				float weight = 0.0f;
				VertexWeight() = default;
				VertexWeight(uint32_t i, float w) :
					index(i), weight(w) { }
			};
			using VertexWeightVector = Vector<VertexWeight>;
			
		public:
			VertexWeightVector& weights()
				{ return _weights; }
			
			const VertexWeightVector& weights() const
				{ return _weights; }
			
			void sortWeights();
			
			size_t linkTag() const
				{ return _linkTag; }
			
			void setLinkTag(size_t lt)
				{ _linkTag = lt; }
			
			s3d::SkeletonElement::Pointer link()
				{ return _link; }
			
			const s3d::SkeletonElement::Pointer link() const
				{ return _link; }
			
			void setLink(s3d::SkeletonElement::Pointer l)
				{ _link = l; }
			
			mat4 transformMatrix();
			
			void setLinkInitialTransform(const mat4& m)
				{ _linkInitialTransformInverse = m.inverse(); }

			void setMeshInitialTransform(const mat4& m)
				{ _meshInitialTransform = m; }
			
			const mat4& linkInitialTransformInverse() const
				{ return _linkInitialTransformInverse; }

			const mat4& meshInitialTransform() const
				{ return _meshInitialTransform; }
			
		private:
			s3d::SkeletonElement::Pointer _link;
			VertexWeightVector _weights;
			mat4 _linkInitialTransformInverse;
			mat4 _meshInitialTransform;
			size_t _linkTag = 0;
		};
		
		class MeshDeformer : public AtomicCounter
		{
		public:
			ET_DECLARE_POINTER(MeshDeformer)
			
			void addCluster(MeshDeformerCluster::Pointer cl)
				{ _clusters.push_back(cl); }
			
			Vector<MeshDeformerCluster::Pointer>& clusters()
				{ return _clusters; }
			
			const Vector<MeshDeformerCluster::Pointer>& clusters() const
				{ return _clusters; }
			
			const Vector<mat4>& calculateTransforms();
			
		private:
			Vector<MeshDeformerCluster::Pointer> _clusters;
			Vector<mat4> _transformMatrices;
		};
	}
}
