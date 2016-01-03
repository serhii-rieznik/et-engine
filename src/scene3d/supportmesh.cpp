/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/primitives/primitives.h>
#include <et/vertexbuffer/vertexstorage.h>
#include <et/scene3d/supportmesh.h>

using namespace et;
using namespace et::s3d;

SupportMesh::SupportMesh(const std::string& name, BaseElement* parent) :
	Mesh(name, parent) { }

SupportMesh::SupportMesh(const std::string& name, const VertexArrayObject& ib, const SceneMaterial::Pointer& material,
	uint32_t start, uint32_t num, const VertexStorage::Pointer& storage, const IndexArray::Pointer& ia, BaseElement* parent) :
	Mesh(name, ib, material, start, num, storage, ia, parent)
{
}

SupportMesh* SupportMesh::duplicate()
{
	SupportMesh* result = etCreateObject<SupportMesh>(name(), vertexArrayObject(),
		material(), startIndex(), numIndexes(), vertexStorage(), indexArray(), parent());

	duplicateMeshPropertiesToMesh(result);
	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	return result;
}
