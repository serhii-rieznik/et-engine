/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/mesh.h>

using namespace et;
using namespace et::s3d;

void MeshDeformerCluster::sortWeights()
{
	std::sort(_weights.begin(), _weights.end(), [](const VertexWeight& l, const VertexWeight& r)
	{
		return (l.index < r.index);
	});
}

const std::vector<mat4>& MeshDeformer::calculateTransformsForMesh(Mesh* mesh)
{
	_transformMatrices.clear();
	_transformMatrices.reserve(std::min(size_t(4), _clusters.size()));
	
	for (auto& cluster : _clusters)
		_transformMatrices.push_back(cluster->transformMatrix());
	
	while (_transformMatrices.size() < 4)
		_transformMatrices.push_back(identityMatrix);
	
	return _transformMatrices;
}

mat4 MeshDeformerCluster::transformMatrix()
{
	return _meshInitialTransform * _linkInitialTransformInverse * _link->finalTransform();
}
