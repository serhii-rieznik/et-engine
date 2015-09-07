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
	class ET_ALIGNED(16) KDTree
	{
	public:
		struct Node
		{
			rt::BoundingBox boundingBox;
			std::vector<size_t> triangles;
			int splitAxis;
			float splitDistance;
			bool containsSubNodes;

			union
			{
				struct
				{
					Node* left;
					Node* right;
				};
				Node* subNodes[2];
			};
			
			Node() :
				left(nullptr), right(nullptr) { };
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
		void printStructure();
		
		const rt::Triangle& triangleAtIndex(size_t) const;
		
	private:
		void printStructure(Node*, const std::string&);
		void cleanUpRecursively(Node*);
		
		Node* buildRootNode();
		void splitNodeUsingBins(Node*, size_t);
		void splitNodeUsingSortedArray(Node*, size_t);
		void buildSplitBoxesUsingAxisAndPosition(Node*, int axis, float position);
		void distributeTrianglesToChildren(Node*);
		
		bool findIntersection(const rt::Ray&, TraverseResult&, KDTree::Node*);
		float findIntersectionInNode(const rt::Ray&, KDTree::Node*, TraverseResult&);
		
	private:
		Node* _root = nullptr;
		size_t _maxDepth = 0;
		size_t _minTrianglesToSubdivide = 16;
		int _spaceSplitSize = 32;
		std::vector<rt::Triangle> _triangles;
	};
}
