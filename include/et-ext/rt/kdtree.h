/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <stack>
#include <et-ext/rt/raytraceobjects.h>

namespace et
{
	class ET_ALIGNED(16) KDTree
	{
	public:
		struct Node
		{
			std::vector<rt::index> triangles;
			rt::index children[2];
			float distance;
			int axis;
		};
		using NodeList = std::vector<Node, SharedBlockAllocatorSTDProxy<Node>>;
		
		struct Stats
		{
			size_t totalTriangles = 0;
			size_t distributedTriangles = 0;
			size_t totalNodes = 0;
			size_t leafNodes = 0;
			size_t emptyLeafNodes = 0;
			size_t maxDepth = 0;
			size_t maxTrianglesPerNode = 0;
			size_t minTrianglesPerNode = std::numeric_limits<size_t>::max();
		};
		
		struct ET_ALIGNED(16) TraverseResult
		{
			rt::float4 intersectionPoint;
			rt::float4 intersectionPointBarycentric;
			size_t triangleIndex = InvalidIndex;
		};
		
		enum class BuildMode
		{
			SortedArrays,
		};

	public:
		~KDTree();
		
        void build(const rt::TriangleList&, size_t maxDepth);
		Stats nodesStatistics() const;
		void cleanUp();
		
		const Node& root() const
			{ return _nodes.front(); }
		
		const Node& nodeAt(size_t i) const
			{ return _nodes.at(i); }

		const rt::BoundingBox& bboxAt(size_t i) const
			{ return _boundingBoxes.at(i); }
		
		TraverseResult traverse(const rt::Ray& r);
		void printStructure();
		
		const rt::Triangle& triangleAtIndex(size_t) const;
		
	private:
		void printStructure(const Node&, const std::string&);
		
		Node buildRootNode();
		void splitNodeUsingSortedArray(size_t, size_t);
		void buildSplitBoxesUsingAxisAndPosition(size_t nodeIndex, int axis, float position);
		void distributeTrianglesToChildren(size_t nodeIndex);
		
		float findIntersectionInNode(const rt::Ray&, const KDTree::Node&, TraverseResult&);
		
	private:
		NodeList _nodes;
		rt::BoundingBoxList _boundingBoxes;
		
		rt::TriangleList _triangles;
		rt::IntersectionDataList _intersectionData;
		
		size_t _maxDepth = 0;
		size_t _maxBuildDepth = 0;
		BuildMode _buildMode = BuildMode::SortedArrays;
	};
    
    template <size_t MaxElements, class T>
    struct ET_ALIGNED(16) FastStack
    {
    public:
        enum : size_t
        {
            MaxElementsPlusOne = MaxElements + 1,
        };
        
    public:
        template <typename ... Args>
        void emplace(Args&&... a)
        {
            ET_ASSERT(_size < MaxElements);
            _elements[_size] = T(std::forward<Args>(a)...);
            ++_size;
        }
        
        void push(const T& value)
        {
            ET_ASSERT(_size < MaxElements);
            _elements[_size] = value;
            ++_size;
        }
        
        bool empty() const
            { return _size == 0; }
        
        bool hasSomething() const
            { return _size > 0; }
        
        const T& top() const
            { ET_ASSERT(_size < MaxElementsPlusOne); return _elements[_size - 1]; }
        
        void pop()
            { ET_ASSERT(_size > 0); --_size; }
        
        size_t size() const
            { return _size; }
        
    private:
        T _elements[MaxElements];
        size_t _size = 0;
    };
}
