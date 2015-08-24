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
			rt::BoundingBox boundingBox;
			std::vector<size_t> triangles;
			Node* left = nullptr;
			Node* right = nullptr;
		};
		
		typedef std::stack<Node*> TraverseResult;
		
	public:
		~KDTree();
		
		void build(const std::vector<rt::Triangle>&, size_t maxDepth, int splits);
		void cleanUp();
		
		Node* root() const
			{ return _root; }
		
		TraverseResult traverse(const rt::Ray&);
		
	private:
		void printStructure(Node*, const std::string&);
		void cleanUpRecursively(Node*);
		
		Node* buildRootNode(const std::vector<rt::Triangle>&);
		void splitNodeUsingBins(Node*, const std::vector<rt::Triangle>&, size_t depth);
		void splitNodeUsingSortedArray(Node*, const std::vector<rt::Triangle>&);
		void buildSplitBoxesUsingAxisAndPosition(Node*, int axis, float position);
		void distributeTrianglesToChildren(Node*, const std::vector<rt::Triangle>&);

		void traverseRecursive(const rt::Ray& ray, Node* node, TraverseResult&);
		
	private:
		Node* _root = nullptr;
		size_t _maxDepth = 0;
		size_t _minTrianglesToSubdivide = 5;
		int _spaceSplitSize = 32;
	};
}
