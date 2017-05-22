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
	ET_DECLARE_POINTER(Mesh);
	
	static const std::string defaultMeshName;

public:
	Mesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
	
	ElementType type() const override
		{ return ElementType::Mesh; }

	Mesh* duplicate() override;
	
	void calculateSupportData();
	
	const Sphere& boundingSphere();
	const Sphere& boundingSphereUntransformed();
	
	const BoundingBox& boundingBox();
	const BoundingBox& tranformedBoundingBox();

	float finalTransformScale();
	
	MeshDeformer::Pointer deformer()
		{ return _deformer; }
	
	void setDeformer(MeshDeformer::Pointer d)
		{ _deformer = d; }
	
	const Vector<mat4>& deformationMatrices();

	bool skinned() const;
	VertexStorage::Pointer bakeDeformations();

	RayIntersection intersectsWorldSpaceRay(const ray3d& ray) override;
	
protected:
	void transformInvalidated() override;
	
	struct SupportData
	{
		Sphere untranfromedBoundingSphere;
		Sphere tranfromedBoundingSphere;
		BoundingBox boundingBox;
		BoundingBox transformedBoundingBox;
		float boundingSphereRadius = 0.0f;
		bool shouldUpdateBoundingBox = true;
		bool shouldUpdateBoundingSphere = true;
		bool shouldUpdateBoundingSphereUntransformed = true;
	};

private:
	MeshDeformer::Pointer _deformer;
	SupportData _supportData;
	Vector<mat4> _undeformedTransformationMatrices;
};
}
}
