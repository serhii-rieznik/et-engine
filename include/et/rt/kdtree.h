/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <stack>
#include <et/rt/raytraceobjects.h>

namespace et
{
	class KDTree
	{
	public:
		struct Node
		{
			size_t depth = 0;
			size_t splitAxis = 0;
			plane splitPlane;
			rt::BoundingBox boundingBox;
			
			Node* left = nullptr;
			Node* right = nullptr;

			std::vector<size_t> triangles;
			
			bool isLeafNode() const
				{ return (left == nullptr) && (right == nullptr); }
		};
		
		struct ET_ALIGNED(16) TraverseResult
		{
			vec4simd intersectionPoint = vec4simd(0.0f);
			vec4simd intersectionPointBarycentric = vec4simd(0.0f);
			Node* node = nullptr;
			size_t triangleIndex = InvalidIndex;
		};
		
	public:
		~KDTree();
		
		void build(const std::vector<rt::Triangle>&, size_t maxDepth, int splits);
		void cleanUp();
		
		Node* root() const
			{ return _root; }
		
		TraverseResult traverse(const rt::Ray& r);
		
		const rt::Triangle& triangleAtIndex(size_t) const;
		
	private:
		void printStructure(Node*, const std::string&);
		void cleanUpRecursively(Node*);
		
		Node* buildRootNode();
		void splitNodeUsingBins(Node*);
		void splitNodeUsingSortedArray(Node*);
		void buildSplitBoxesUsingAxisAndPosition(Node*, int axis, float position);
		void distributeTrianglesToChildren(Node*);
		
		bool findIntersection(const rt::Ray&, TraverseResult&, KDTree::Node*);
		float findIntersectionInNode(const rt::Ray&, KDTree::Node*, TraverseResult&);
		
	private:
		Node* _root = nullptr;
		size_t _maxDepth = 0;
		size_t _minTrianglesToSubdivide = 5;
		int _spaceSplitSize = 32;
		std::vector<rt::Triangle> _triangles;
	};
}
