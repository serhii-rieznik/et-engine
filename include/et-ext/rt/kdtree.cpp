/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/kdtree.h>
#include <et/core/tools.h>

namespace et
{
namespace rt
{

const size_t DepthLimit = 128;
const size_t MinTrianglesToSubdivide = 12;

struct Split
{
	float3 cost = float3(0.0f);
	int index = 0;
	int axis = 0;
};

KDTree::~KDTree()
{
	cleanUp();
}

rt::KDTree::Node KDTree::buildRootNode()
{
	_intersectionData.reserve(_triangles.size());
	
	float4 minVertex = _triangles.front().v[0];
	float4 maxVertex = minVertex;
	
	for (const auto& t : _triangles)
	{
		minVertex = minVertex.minWith(t.v[0]);
		minVertex = minVertex.minWith(t.v[1]);
		minVertex = minVertex.minWith(t.v[2]);
		maxVertex = maxVertex.maxWith(t.v[0]);
		maxVertex = maxVertex.maxWith(t.v[1]);
		maxVertex = maxVertex.maxWith(t.v[2]);
		
		_intersectionData.emplace_back(t.v[0], t.edge1to0, t.edge2to0);
	}
	
	float4 center = (minVertex + maxVertex) * float4(0.5f);
	float4 halfSize = (maxVertex - minVertex) * float4(0.5f);
    _sceneBoundingBox = BoundingBox(center, halfSize);
	_indices.reserve(10 * _triangles.size());
    _boundingBoxes.emplace_back(_sceneBoundingBox);
	
	KDTree::Node result;
	result.endIndex = static_cast<index>(_triangles.size());
	for (index i = 0; i < result.endIndex; ++i)
	{
		_indices.emplace_back(i);
	}
	return result;
}

void KDTree::build(const TriangleList& triangles, size_t maxDepth)
{
	cleanUp();
	
	_maxBuildDepth = 0;
	_triangles = triangles;
	
	_maxDepth = std::min(DepthLimit, maxDepth);
	_nodes.reserve(maxDepth * maxDepth);
	_nodes.emplace_back(buildRootNode());

	uint64_t t0 = queryContiniousTimeInMilliSeconds();
	splitNodeUsingSortedArray(0, 0);
	uint64_t t1 = queryContiniousTimeInMilliSeconds();
	log::info("kD-tree building time: %llu", t1 - t0);
}

void KDTree::buildSplitBoxesUsingAxisAndPosition(size_t nodeIndex, int axis, float_type position)
{
	auto bbox = _boundingBoxes.at(nodeIndex);
	
	float4 lowerCorner = bbox.minVertex();
	float4 upperCorner = bbox.maxVertex();
	
	vec4 axisScale4(1.0f);
	axisScale4[axis] = 0.0f;
	
	vec4 posScale4(0.0f);
	posScale4[axis] = 1.0f;
	
	float4 axisScale(axisScale4);
	float4 posScale(posScale4);
	
	float4 middlePoint = lowerCorner * axisScale + posScale * position;
	float4 leftSize = (middlePoint - lowerCorner) * posScale * 0.5f;
	float4 rightSize = (upperCorner - middlePoint) * posScale * 0.5f;
	
	_nodes.at(nodeIndex).axis = axis;
	_nodes.at(nodeIndex).distance = position;
	_nodes.at(nodeIndex).children[0] = static_cast<index>(_nodes.size());
	_nodes.emplace_back();
	_nodes.back().children[0] = InvalidIndex;
	_nodes.back().children[1] = InvalidIndex;
	_nodes.back().axis = InvalidIndex;
	_nodes.back().distance = 0.0f;

	_boundingBoxes.emplace_back(bbox.center * axisScale + posScale * (middlePoint - leftSize),
		bbox.halfSize * axisScale + posScale * leftSize);

	_nodes.at(nodeIndex).children[1] = static_cast<index>(_nodes.size());
	_nodes.emplace_back();
	_nodes.back().children[0] = InvalidIndex;
	_nodes.back().children[1] = InvalidIndex;
	_nodes.back().axis = InvalidIndex;
	_nodes.back().distance = 0.0f;

	_boundingBoxes.emplace_back(bbox.center * axisScale + posScale * (middlePoint + rightSize),
		bbox.halfSize * axisScale + posScale * rightSize);
}

void KDTree::distributeTrianglesToChildren(size_t nodeIndex)
{
	ET_ALIGNED(16) vec4 minVertex;
	ET_ALIGNED(16) vec4 maxVertex;
	
	auto& node = _nodes.at(nodeIndex);

	static Vector<index> rightIndexes;
	rightIndexes.reserve(32 * 1024);
	rightIndexes.clear();
	
	static Vector<index> leftIndexes;
	leftIndexes.reserve(32 * 1024);
	leftIndexes.clear();

	for (index i = node.startIndex, e = node.startIndex + node.numIndexes(); i < e; ++i)
	{
		index triIndex = _indices[i];
		const auto& tri = _triangles.at(triIndex);
		tri.minVertex().loadToFloats(minVertex.data());
		tri.maxVertex().loadToFloats(maxVertex.data());
		
		if (minVertex[node.axis] > node.distance)
		{
			rightIndexes.emplace_back(triIndex);
		}
		else if (maxVertex[node.axis] < node.distance)
		{
			leftIndexes.emplace_back(triIndex);
		}
		else
		{
			rightIndexes.emplace_back(triIndex);
			leftIndexes.emplace_back(triIndex);
		}
	}

	auto& left = _nodes.at(node.children[0]);
	left.startIndex = static_cast<index>(_indices.size());
	left.endIndex = left.startIndex + static_cast<index>(leftIndexes.size());
	_indices.insert(_indices.end(), leftIndexes.begin(), leftIndexes.end());

	auto& right = _nodes.at(node.children[1]);
	right.startIndex = static_cast<index>(_indices.size());
	right.endIndex = right.startIndex + static_cast<index>(rightIndexes.size());
	_indices.insert(_indices.end(), rightIndexes.begin(), rightIndexes.end());
}

void KDTree::cleanUp()
{
	_nodes.clear();
	_triangles.clear();
}

void KDTree::splitNodeUsingSortedArray(size_t nodeIndex, size_t depth)
{
	auto numTriangles = _nodes.at(nodeIndex).numIndexes();
	if ((depth > _maxDepth) || (numTriangles < MinTrianglesToSubdivide))
	{
		return;
	}
		
	_maxBuildDepth = std::max(_maxBuildDepth, depth);
	const auto& bbox = _boundingBoxes.at(nodeIndex);
	
	auto estimateCostAtSplit = [&bbox, nodeIndex, this](float_type splitPlane, size_t leftTriangles,
		size_t rightTriangles, int axis) -> float
	{
		ET_ASSERT((leftTriangles + rightTriangles) == _nodes.at(nodeIndex).numIndexes());
		
		const vec4& minVertex = bbox.minVertex().toVec4();
		if (splitPlane <= minVertex[axis] + Constants::epsilon)
			return std::numeric_limits<float>::max();

		const vec4& maxVertex = bbox.maxVertex().toVec4();
		if (splitPlane >= maxVertex[axis] - Constants::epsilon)
			return std::numeric_limits<float>::max();

		vec4 axisScale(1.0f);
		vec4 axisOffset(0.0f);
		axisScale[axis] = 0.0f;
		axisOffset[axis] = splitPlane;
		BoundingBox leftBox(bbox.minVertex(), bbox.maxVertex() * float4(axisScale) + float4(axisOffset), 0);
		BoundingBox rightBox(bbox.minVertex() * float4(axisScale) + float4(axisOffset), bbox.maxVertex(), 0);
		
		float_type totalSquare = bbox.square();
		float_type leftSquare = leftBox.square() / totalSquare;
		float_type rightSquare = rightBox.square() / totalSquare;
		float_type costLeft = leftSquare * float(leftTriangles);
		float_type costRight = rightSquare * float(rightTriangles);
		return costLeft + costRight;
	};
	
	auto compareAndAssignMinimum = [](float& minCost, float_type cost) -> bool
	{
		bool result = (cost < minCost);
		if (result)
			minCost = cost;
		return result;
	};
	
	const auto& localNode = _nodes.at(nodeIndex);

	static Vector<float3> minPoints;
	static Vector<float3> maxPoints;
	minPoints.reserve(32 * 1024);
	maxPoints.reserve(32 * 1024);

	minPoints.clear();
	maxPoints.clear();
	for (index i = localNode.startIndex, e = localNode.endIndex; i < e; ++i)
	{
		const auto& tri = _triangles.at(_indices[i]);
		minPoints.emplace_back(tri.minVertex().xyz() - float3(Constants::epsilon));
		maxPoints.emplace_back(tri.maxVertex().xyz() + float3(Constants::epsilon));
	}
	
	float3 splitPosition = minPoints.at(minPoints.size() / 2);
	float3 splitCost(Constants::initialSplitValue);
	
	bool splitFound = false;
	int numElements = static_cast<int>(minPoints.size());

	static int currentAxis = 0;
	for (currentAxis = 0; currentAxis < 3; ++currentAxis)
	{
		std::sort(minPoints.begin(), minPoints.end(), [](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });
		
		std::sort(maxPoints.begin(), maxPoints.end(), [](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });

		for (int i = 1; i + 1 < numElements; ++i)
		{
			float_type costMin = estimateCostAtSplit(minPoints.at(i)[currentAxis], i, numElements - i, currentAxis);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMin))
			{
				splitPosition[currentAxis] = minPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
		
		for (int i = numElements - 2; i > 0; --i)
		{
			float_type costMax = estimateCostAtSplit(maxPoints.at(i)[currentAxis], i, numElements - i, currentAxis);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMax))
			{
				splitPosition[currentAxis] = maxPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
	}
	
	float_type targetValue = std::min(splitCost.x, std::min(splitCost.y, splitCost.z));
	for (int currentAxis = 0; splitFound && (currentAxis < 3); ++currentAxis)
	{
		if (splitCost[currentAxis] == targetValue)
		{
			buildSplitBoxesUsingAxisAndPosition(nodeIndex, currentAxis, splitPosition[currentAxis]);
			distributeTrianglesToChildren(nodeIndex);
			splitNodeUsingSortedArray(_nodes.at(nodeIndex).children[0], depth + 1);
			splitNodeUsingSortedArray(_nodes.at(nodeIndex).children[1], depth + 1);
			break;
		}
	}
}

void KDTree::printStructure()
{
	printStructure(_nodes.front(), std::string());
}

void KDTree::printStructure(const Node& node, const std::string& tag)
{
	const char* axis[] = { "X", "Y", "Z" };
	if (node.axis <= MaxAxisIndex)
	{
		log::info("%s %s, %.2f", tag.c_str(), axis[node.axis], node.distance);
		printStructure(_nodes.at(node.children[0]), tag + "--|");
		printStructure(_nodes.at(node.children[1]), tag + "--|");
	}
	else
	{
		log::info("%s %u tris", tag.c_str(), node.numIndexes());
	}
}

const Triangle& KDTree::triangleAtIndex(size_t i) const
{
	return _triangles.at(i);
}

struct KDTreeSearchNode
{
	index ind;
    float_type time;
    
    KDTreeSearchNode() = default;
	KDTreeSearchNode(index n, float_type t) :
        ind(n), time(t) { }
};

KDTree::TraverseResult KDTree::traverse(const Ray& ray)
{
	KDTree::TraverseResult result;
	
    float_type eps = Constants::epsilon;

	float_type tNear = 0.0f;
	float_type tFar = 0.0f;
	
	if (!rayToBoundingBox(ray, _sceneBoundingBox, tNear, tFar))
		return result;
	
	if (tNear < 0.0f)
		tNear = 0.0f;

	ET_ALIGNED(16) float_type direction[4];
	ray.direction.reciprocal().loadToFloats(direction);

	ET_ALIGNED(16) float_type originDivDirection[4];
	(ray.origin / (ray.direction + rt::float4(std::numeric_limits<float>::epsilon()))).loadToFloats(originDivDirection);
    
	Node localNode = _nodes.front();
	FastStack<DepthLimit + 1, KDTreeSearchNode> traverseStack;
	for (;;)
	{
		while (localNode.axis <= MaxAxisIndex)
		{
			int side = floatIsNegative(direction[localNode.axis]);
			float_type tSplit = localNode.distance * direction[localNode.axis] - originDivDirection[localNode.axis];
            
			if (tSplit < tNear)
			{
				localNode = _nodes[localNode.children[1 - side]];
			}
			else if (tSplit > tFar)
			{
				localNode = _nodes[localNode.children[side]];
			}
			else
			{
				traverseStack.emplace(localNode.children[1 - side], tFar);
				localNode = _nodes[localNode.children[side]];
				tFar = tSplit;
			}
		}

		if (localNode.nonEmpty())
		{
			result.triangleIndex = InvalidIndex;

			ET_ALIGNED(16) float_type minDistance = std::numeric_limits<float>::max();
			for (index i = localNode.startIndex, e = localNode.endIndex; i < e; ++i)
			{
				index triangleIndex = _indices[i];
				IntersectionData data = _intersectionData[triangleIndex];
				
				float4 pvec = ray.direction.crossXYZ(data.edge2to0);
				union
				{
					float f;
					uint32_t i;
				} det = { data.edge1to0.dot(pvec) };

				if (!(det.i & 0x7fffffff))
					continue;

				float inv_dev = 1.0f / det.f;

				float4 tvec = ray.origin - data.v0;
				float u = tvec.dot(pvec) * inv_dev;
				if ((u < 0.0f) || (u > 1.0f))
					continue;

				float4 qvec = tvec.crossXYZ(data.edge1to0);
				float v = ray.direction.dot(qvec) * inv_dev;
				float uv = u + v;
				if ((v < 0.0f) || (uv > 1.0f))
					continue;

				float t = data.edge2to0.dot(qvec) * inv_dev;
				if ((t < minDistance) && (t <= tFar) && (t > Constants::distanceEpsilon))
				{
					minDistance = t;
					result.triangleIndex = triangleIndex;
					result.intersectionPointBarycentric = float4(1.0f - uv, u, v, 0.0f);
				}
			}

			if (result.triangleIndex < InvalidIndex)
			{
				result.intersectionPoint = ray.origin + ray.direction * minDistance;
				return result;
			}
		}
		
		if (traverseStack.empty())
		{
			result.triangleIndex = InvalidIndex;
			return result;
		}
		
		localNode = _nodes[traverseStack.top().ind];
        tNear = tFar - eps;
		tFar = traverseStack.top().time + eps;

		traverseStack.pop();
	}
	
	return result;
}

KDTree::Stats KDTree::nodesStatistics() const
{
	KDTree::Stats result;
	result.totalNodes = _nodes.size();
	result.maxDepth = _maxBuildDepth;
	result.totalTriangles = _triangles.size();
	for (const auto& node : _nodes)
	{
		if (node.axis == InvalidIndex)
		{
			++result.leafNodes;

			if (node.empty())
				++result.emptyLeafNodes;
		}
		
		if ((node.children[0] == InvalidIndex) && (node.children[1] == InvalidIndex) && (node.numIndexes() > 0))
		{
			result.maxTrianglesPerNode = std::max(result.maxTrianglesPerNode, node.numIndexes());
			result.minTrianglesPerNode = std::min(result.minTrianglesPerNode, node.numIndexes());
		}
		
		result.distributedTriangles += node.numIndexes();
	}
	return result;
}

}
}
