/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rt/kdtree.h>

using namespace et;

KDTree::~KDTree()
{
	cleanUp();
}

void KDTree::build(const std::vector<rt::Triangle>& triangles)
{
	cleanUp();
	
	vec4simd minVertex = triangles.front().v[0];
	vec4simd maxVertex = minVertex;
	
	for (const auto& t : triangles)
	{
		minVertex = minVertex.minWith(t.v[0]);
		minVertex = minVertex.minWith(t.v[1]);
		minVertex = minVertex.minWith(t.v[2]);
		maxVertex = maxVertex.maxWith(t.v[0]);
		maxVertex = maxVertex.maxWith(t.v[1]);
		maxVertex = maxVertex.maxWith(t.v[2]);
	}
	
	vec4simd center = (minVertex + maxVertex) * vec4simd(0.5f);
	vec4simd halfSize = (maxVertex - minVertex) * vec4simd(0.5f);
	
	_root = sharedObjectFactory().createObject<KDTree::Node>();
	_root->boundingBox = rt::BoundingBox(center, halfSize);
	_root->triangles.reserve(triangles.size());
	for (size_t i = 0, e = triangles.size(); i < e; ++i)
		_root->triangles.push_back(i);
}

void KDTree::cleanUp()
{
	cleanUpRecursively(_root);
	_root = nullptr;
}

void KDTree::cleanUpRecursively(Node* node)
{
	if (node == nullptr) return;
	
	cleanUpRecursively(node->left);
	cleanUpRecursively(node->right);
	sharedObjectFactory().deleteObject(node);
}

KDTree::Node* KDTree::traverse(const rt::Ray& ray)
{
	if (rt::rayHitsBoundingBox(ray, _root->boundingBox))
		return _root;
	
	return nullptr;
}
