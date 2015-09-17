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
			union
			{
				struct
				{
					Node* left;
					Node* right;
				};
				Node* subNodes[2];
			};
			
			rt::BoundingBox boundingBox;
			std::vector<size_t> triangles;
			float splitDistance;
			int splitAxis : 3;
			int containsSubNodes : 1;
			
			Node() :
				left(nullptr), right(nullptr) { };
		};
		
		struct ET_ALIGNED(16) TraverseResult
		{
			rt::float4 intersectionPoint;
			rt::float4 intersectionPointBarycentric;
			size_t triangleIndex = InvalidIndex;
		};
		
		enum class BuildMode
		{
			Bins,
			SortedArrays,
			BruteForce
		};

		using TriangleList = std::vector<rt::Triangle, SharedBlockAllocatorSTDProxy<rt::Triangle>>;
		
	public:
		~KDTree();
		
		void build(const TriangleList&, size_t maxDepth, int splits);
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
		
		float findIntersectionInNode(const rt::Ray&, KDTree::Node*, TraverseResult&);
		
	private:
		Node* _root = nullptr;
		std::vector<rt::Triangle> _triangles;
		size_t _maxDepth = 0;
		size_t _minTrianglesToSubdivide = 16;
		int _spaceSplitSize = 32;
		BuildMode _buildMode = BuildMode::SortedArrays;
	};
}
