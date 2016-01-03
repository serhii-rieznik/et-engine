/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rt/kdtree.h>

using namespace et;

namespace
{
	const size_t DepthLimit = 31;
	const size_t MinTrianglesToSubdivide = 16;
	const size_t MaxTraverseStack = DepthLimit + 1;
	
	struct Split
	{
		vec3 cost = vec3(0.0f);
		int index = 0;
		int axis = 0;
	};
	
	struct FastTraverseStack
	{
	public:
		FastTraverseStack()
		{
			memset(nodes, 0, sizeof(nodes));
			memset(times, 0, sizeof(times));
		}
		
		void emplace(rt::index nodeIndex, float farTime)
		{
			ET_ASSERT(size < MaxTraverseStack);
			nodes[size] = nodeIndex;
			times[size] = farTime;
			++size;
		}
		
		bool empty() const
			{ return size == 0; }
		
		rt::index topNodeIndex() const
			{ ET_ASSERT(size < MaxTraverseStack + 1); return nodes[size - 1]; }
		
		const float topTime() const
			{ ET_ASSERT(size < MaxTraverseStack + 1); return times[size - 1]; }
		
		void pop()
			{ ET_ASSERT(size > 0); --size; }
		
	private:
		rt::index nodes[MaxTraverseStack];
		float times[MaxTraverseStack];
		
		size_t size = 0;
	};
}

KDTree::~KDTree()
{
	cleanUp();
}

KDTree::Node KDTree::buildRootNode()
{
	_intersectionData.reserve(_triangles.size());
	
	rt::float4 minVertex = _triangles.front().v[0];
	rt::float4 maxVertex = minVertex;
	
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
	
	rt::float4 center = (minVertex + maxVertex) * rt::float4(0.5f);
	rt::float4 halfSize = (maxVertex - minVertex) * rt::float4(0.5f);
	_boundingBoxes.emplace_back(center, halfSize);
	
	KDTree::Node result;
	result.children[0] = rt::InvalidIndex;
	result.children[1] = rt::InvalidIndex;
	result.axis = -1;
	result.distance = 0.0f;
	result.triangles.reserve(_triangles.size());
	for (rt::index i = 0, e = static_cast<rt::index>(_triangles.size()); i < e; ++i)
		result.triangles.push_back(i);
	return result;
}

void KDTree::build(const rt::TriangleList& triangles, size_t maxDepth, int splits)
{
	cleanUp();
	
	_maxBuildDepth = 0;
	_triangles = triangles;
	_spaceSplitSize = splits;
	
	_maxDepth = etMin(DepthLimit, maxDepth);
	_nodes.reserve(maxDepth * maxDepth);
	_nodes.push_back(buildRootNode());
	
	switch (_buildMode)
	{
		case BuildMode::SortedArrays:
		{
			splitNodeUsingSortedArray(0, 0);
			break;
		}
		default:
			ET_FAIL("Invalid kd-tree build mode");
	}
}

vec3 triangleCentroid(const rt::Triangle& t)
{
	rt::float4 maxV = t.v[0].maxWith(t.v[1].maxWith(t.v[2]));
	rt::float4 minV = t.v[0].minWith(t.v[1].minWith(t.v[2]));
	return ((maxV + minV) * 0.5f).xyz();
}

void KDTree::buildSplitBoxesUsingAxisAndPosition(size_t nodeIndex, int axis, float position)
{
	auto bbox = _boundingBoxes.at(nodeIndex);
	
	rt::float4 lowerCorner = bbox.minVertex();
	rt::float4 upperCorner = bbox.maxVertex();
	
	vec4 axisScale4(1.0f);
	axisScale4[axis] = 0.0f;
	
	vec4 posScale4(0.0f);
	posScale4[axis] = 1.0f;
	
	rt::float4 axisScale(axisScale4);
	rt::float4 posScale(posScale4);
	
	rt::float4 middlePoint = lowerCorner * axisScale + posScale * position;
	rt::float4 leftSize = (middlePoint - lowerCorner) * posScale * 0.5f;
	rt::float4 rightSize = (upperCorner - middlePoint) * posScale * 0.5f;
	
	_nodes.at(nodeIndex).axis = axis;
	_nodes.at(nodeIndex).distance = position;
	_nodes.at(nodeIndex).children[0] = static_cast<rt::index>(_nodes.size());
	_nodes.emplace_back();
	_nodes.back().children[0] = rt::InvalidIndex;
	_nodes.back().children[1] = rt::InvalidIndex;
	_nodes.back().axis = -1;
	_nodes.back().distance = 0.0f;

	_boundingBoxes.emplace_back(bbox.center * axisScale + posScale * (middlePoint - leftSize),
		bbox.halfSize * axisScale + posScale * leftSize);

	_nodes.at(nodeIndex).children[1] = static_cast<rt::index>(_nodes.size());
	_nodes.emplace_back();
	_nodes.back().children[0] = rt::InvalidIndex;
	_nodes.back().children[1] = rt::InvalidIndex;
	_nodes.back().axis = -1;
	_nodes.back().distance = 0.0f;

	_boundingBoxes.emplace_back(bbox.center * axisScale + posScale * (middlePoint + rightSize),
		bbox.halfSize * axisScale + posScale * rightSize);
}

void KDTree::distributeTrianglesToChildren(size_t nodeIndex)
{
	ET_ALIGNED(16) vec4 minVertex;
	ET_ALIGNED(16) vec4 maxVertex;
	
	auto& node = _nodes.at(nodeIndex);
	auto& left = _nodes.at(node.children[0]);
	auto& right = _nodes.at(node.children[1]);
	
	for (auto triIndex : node.triangles)
	{
		const auto& tri = _triangles.at(triIndex);
		tri.minVertex().loadToFloats(minVertex.data());
		tri.maxVertex().loadToFloats(maxVertex.data());
		
		if (minVertex[node.axis] > node.distance)
		{
			right.triangles.push_back(triIndex);
		}
		else if (maxVertex[node.axis] < node.distance)
		{
			left.triangles.push_back(triIndex);
		}
		else
		{
			left.triangles.push_back(triIndex);
			right.triangles.push_back(triIndex);
		}
	}
	
	std::vector<rt::index> emptyVector;
	_nodes.at(nodeIndex).triangles.swap(emptyVector);
}

void KDTree::cleanUp()
{
	_nodes.clear();
	_triangles.clear();
	_spaceSplitSize = 0;
}

void KDTree::splitNodeUsingSortedArray(size_t nodeIndex, size_t depth)
{
	auto numTriangles = _nodes.at(nodeIndex).triangles.size();
	if ((depth > _maxDepth) || (numTriangles < MinTrianglesToSubdivide))
	{
		return;
	}
		
	_maxBuildDepth = etMax(_maxBuildDepth, depth);
	const auto& bbox = _boundingBoxes.at(nodeIndex);
	
	auto estimateCostAtSplit = [&bbox, nodeIndex, this](float splitPlane, size_t leftTriangles,
		size_t rightTriangles, int axis) -> float
	{
		ET_ASSERT((leftTriangles + rightTriangles) == _nodes.at(nodeIndex).triangles.size());
		
		const vec4& minVertex = bbox.minVertex().toVec4();
		if (splitPlane <= minVertex[axis] + rt::Constants::epsilon)
			return std::numeric_limits<float>::max();

		const vec4& maxVertex = bbox.maxVertex().toVec4();
		if (splitPlane >= maxVertex[axis] - rt::Constants::epsilon)
			return std::numeric_limits<float>::max();

		vec4 axisScale(1.0f);
		vec4 axisOffset(0.0f);
		axisScale[axis] = 0.0f;
		axisOffset[axis] = splitPlane;
		rt::BoundingBox leftBox(bbox.minVertex(), bbox.maxVertex() * rt::float4(axisScale) + rt::float4(axisOffset), 0);
		rt::BoundingBox rightBox(bbox.minVertex() * rt::float4(axisScale) + rt::float4(axisOffset), bbox.maxVertex(), 0);
		
		float totalSquare = bbox.square();
		float leftSquare = leftBox.square() / totalSquare;
		float rightSquare = rightBox.square() / totalSquare;
		float costLeft = leftSquare * float(leftTriangles);
		float costRight = rightSquare * float(rightTriangles);
		return costLeft + costRight;
	};
	
	auto compareAndAssignMinimum = [](float& minCost, float cost) -> bool
	{
		bool result = (cost < minCost);
		if (result)
			minCost = cost;
		return result;
	};
	
	std::vector<vec3> minPoints;
	std::vector<vec3> maxPoints;
	const auto& localNode = _nodes.at(nodeIndex);
	for (size_t triIndex : localNode.triangles)
	{
		const auto& tri = _triangles.at(triIndex);
		minPoints.push_back(tri.minVertex().xyz() - vec3(rt::Constants::epsilon));
		maxPoints.push_back(tri.maxVertex().xyz() + vec3(rt::Constants::epsilon));
	}
	
	vec3 splitPosition = minPoints.at(minPoints.size() / 2);
	vec3 splitCost(rt::Constants::initialSplitValue);
	
	bool splitFound = false;
	int numElements = static_cast<int>(minPoints.size());
	
	for (int currentAxis = 0; currentAxis < 3; ++currentAxis)
	{
		std::sort(minPoints.begin(), minPoints.end(), [&currentAxis](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });
		
		std::sort(maxPoints.begin(), maxPoints.end(), [&currentAxis](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });
		
		for (int i = 1; i + 1 < numElements; ++i)
		{
			float costMin = estimateCostAtSplit(minPoints.at(i)[currentAxis], i, numElements - i, currentAxis);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMin))
			{
				splitPosition[currentAxis] = minPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
		
		for (int i = numElements - 2; i > 0; --i)
		{
			float costMax = estimateCostAtSplit(maxPoints.at(i)[currentAxis], i, numElements - i, currentAxis);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMax))
			{
				splitPosition[currentAxis] = maxPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
	}
	
	float targetValue = etMin(splitCost.x, etMin(splitCost.y, splitCost.z));
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
	if (node.axis >= 0)
	{
		log::info("%s %s, %.2f", tag.c_str(), axis[node.axis], node.distance);
		printStructure(_nodes.at(node.children[0]), tag + "--|");
		printStructure(_nodes.at(node.children[1]), tag + "--|");
	}
	else
	{
		log::info("%s %llu tris", tag.c_str(), uint64_t(node.triangles.size()));
	}
}

float KDTree::findIntersectionInNode(const rt::Ray& ray, const KDTree::Node& node, TraverseResult& result)
{
	result.triangleIndex = InvalidIndex;
	
	const rt::index localBufferSize = 32;
	auto trianglesIndices = node.triangles.data();
	
	rt::index remainingIndices = rt::index(node.triangles.size());
	float minDistance = std::numeric_limits<float>::max();
	
	while (remainingIndices > 0)
	{
		rt::index localIndices[localBufferSize];
		rt::index indicesToCopy = etMin(localBufferSize, remainingIndices);
		std::copy(trianglesIndices, trianglesIndices + indicesToCopy, localIndices);
		remainingIndices -= indicesToCopy;
		trianglesIndices += indicesToCopy;
		
		for (rt::index i = 0; i < indicesToCopy; ++i)
		{
			auto triangleIndex = localIndices[i];
			const auto& data = _intersectionData[triangleIndex];
			
			rt::float4 pvec = ray.direction.crossXYZ(data.edge2to0);
			float det = data.edge1to0.dot(pvec);
			if (det * det > rt::Constants::epsilonSquared)// rt::floatIsPositive(det))
			{
				rt::float4 tvec = ray.origin - data.v0;
				float u = tvec.dot(pvec) / det;
				if ((u > rt::Constants::minusEpsilon) && (u < rt::Constants::onePlusEpsilon))
				{
					rt::float4 qvec = tvec.crossXYZ(data.edge1to0);
					float v = ray.direction.dot(qvec) / det;
					float uv = u + v;
					if ((v > rt::Constants::minusEpsilon) && (uv < rt::Constants::onePlusEpsilon))
					{
						float intersectionDistance = data.edge2to0.dot(qvec) / det;
						if ((intersectionDistance > rt::Constants::epsilon) && (intersectionDistance < minDistance))
						{
							result.triangleIndex = triangleIndex;
							minDistance = intersectionDistance;
							result.intersectionPointBarycentric = rt::float4(1.0f - uv, u, v, 0.0f);
						}
					}
				}
			}
		}
	}
	
	if (result.triangleIndex < InvalidIndex)
		result.intersectionPoint = ray.origin + ray.direction * minDistance;
	
	return minDistance;
}

const rt::Triangle& KDTree::triangleAtIndex(size_t i) const
{
	return _triangles.at(i);
}

KDTree::TraverseResult KDTree::traverse(const rt::Ray& r)
{
	KDTree::TraverseResult result;
	
	float tNear = 0.0f;
	float tFar = 0.0f;
	
	if (!rt::rayToBoundingBox(r, _boundingBoxes.front(), tNear, tFar))
		return result;
	
	if (tNear < 0.0f)
		tNear = 0.0f;
	
	size_t currentNode = 0;
	
	ET_ALIGNED(16) float origin[4];
	ET_ALIGNED(16) float direction[4];
	r.origin.loadToFloats(origin);
	r.direction.loadToFloats(direction);
	
	FastTraverseStack traverseStack;
	for (;;)
	{
		while (_nodes[currentNode].axis >= 0)
		{
			const auto& node = _nodes[currentNode];
			
			int axis = node.axis;
			int side = rt::floatIsNegative(direction[axis]);
			
			float tSplit = (node.distance - origin[axis]) / direction[axis];
			
			if (tSplit <= tNear - rt::Constants::epsilon)
			{
				currentNode = node.children[1 - side];
			}
			else if (tSplit >= tFar + rt::Constants::epsilon)
			{
				currentNode = node.children[side];
			}
			else
			{
				traverseStack.emplace(node.children[1 - side], tFar);
				currentNode = node.children[side];
				tFar = tSplit;
			}
		}
		
		const auto& node = _nodes[currentNode];
		if (!node.triangles.empty())
		{
			float tHit = findIntersectionInNode(r, node, result);
			if (tHit <= tFar + rt::Constants::epsilon)
				return result;
		}
		
		if (traverseStack.empty())
		{
			result.triangleIndex = InvalidIndex;
			return result;
		}
		
		currentNode = traverseStack.topNodeIndex();
		tNear = tFar;
		tFar = traverseStack.topTime();
		
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
	
	uint32_t index = 0;
	for (const auto& node : _nodes)
	{
		size_t nodeTriangles = node.triangles.size();
		if (node.axis == -1)
		{
			++result.leafNodes;

			if (nodeTriangles == 0)
				++result.emptyLeafNodes;
		}
		
		if (nodeTriangles > 0)
		{
			result.maxTrianglesPerNode = etMax(result.maxTrianglesPerNode, nodeTriangles);
			result.minTrianglesPerNode = etMin(result.minTrianglesPerNode, nodeTriangles);
		}
		
		result.distributedTriangles += nodeTriangles;
		
		++index;
	}
	return result;
}

