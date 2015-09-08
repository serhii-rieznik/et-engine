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

KDTree::Node* KDTree::buildRootNode()
{
	vec4simd minVertex = _triangles.front().v[0];
	vec4simd maxVertex = minVertex;
	
	for (const auto& t : _triangles)
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
	
	KDTree::Node* result = sharedObjectFactory().createObject<KDTree::Node>();
	result->boundingBox = rt::BoundingBox(center, halfSize);
	result->triangles.reserve(_triangles.size());
	for (size_t i = 0, e = _triangles.size(); i < e; ++i)
		result->triangles.push_back(i);
	
	return result;
}

void KDTree::build(const std::vector<rt::Triangle>& triangles, size_t maxDepth, int splits)
{
	cleanUp();
	
	_triangles = triangles;
	_maxDepth = maxDepth;
	_spaceSplitSize = splits;
	
	_root = buildRootNode();
	//splitNodeUsingSortedArray(_root, 0);
	splitNodeUsingBins(_root, 0);
}

vec3 triangleCentroid(const rt::Triangle& t)
{
	vec4simd maxV = t.v[0].maxWith(t.v[1].maxWith(t.v[2]));
	vec4simd minV = t.v[0].minWith(t.v[1].minWith(t.v[2]));
	return ((maxV + minV) * 0.5f).xyz();
}

struct BinContainer
{
	int numTriangles = 0;
	vec3 center = vec3(0.0f);
};
typedef std::vector<BinContainer> BinContainerVector;

struct Split
{
	vec3 cost = vec3(0.0f);
	int index = 0;
	int axis = 0;
};

void KDTree::splitNodeUsingBins(Node* node, size_t depth)
{
	if ((depth > _maxDepth) || (node->triangles.size() < _minTrianglesToSubdivide))
		return;
		
	const auto& bbox = node->boundingBox;
	
	BinContainerVector binsX(_spaceSplitSize);
	BinContainerVector binsY(_spaceSplitSize);
	BinContainerVector binsZ(_spaceSplitSize);
	const BinContainerVector* bins[3] = { &binsX, &binsY, &binsZ };
	
	vec3 lowerCorner = (bbox.center - bbox.halfSize).xyz();
	vec3 binHalfSize = bbox.halfSize.xyz() / vec3(static_cast<float>(_spaceSplitSize));
	vec3 center = lowerCorner + binHalfSize;
	
	for (int i = 0; i < _spaceSplitSize; ++i)
	{
		auto& binX = binsX[i];
		auto& binY = binsY[i];
		auto& binZ = binsZ[i];
		binX.center = bbox.center.xyz();
		binY.center = bbox.center.xyz();
		binZ.center = bbox.center.xyz();
		binX.center.x = center.x;
		binY.center.y = center.y;
		binZ.center.z = center.z;
		center += 2.0f * binHalfSize;
	}
	
	auto placePointToBins = [this, &binsX, &binsY, &binsZ, &bbox](const vec3& point)
	{
		if (rt::pointInsideBoundingBox(vec4simd(point, 1.0f), bbox))
		{
			vec3 bboxStartPosition = (bbox.center - bbox.halfSize).xyz();
			vec3 centerInBbox = point - bboxStartPosition;
			vec3 centerInBboxRelativeSize = centerInBbox / (2.0f * bbox.halfSize.xyz());
			vec3 centerInSplitSize = vec3(float(_spaceSplitSize - 1)) * centerInBboxRelativeSize;
			vec3i centerInSplitCoordinates(static_cast<int>(centerInSplitSize.x),
				static_cast<int>(centerInSplitSize.y), static_cast<int>(centerInSplitSize.z));
			
			ET_ASSERT(centerInSplitCoordinates.x >= 0);
			ET_ASSERT(centerInSplitCoordinates.y >= 0);
			ET_ASSERT(centerInSplitCoordinates.z >= 0);
			ET_ASSERT(centerInSplitCoordinates.x < _spaceSplitSize);
			ET_ASSERT(centerInSplitCoordinates.y < _spaceSplitSize);
			ET_ASSERT(centerInSplitCoordinates.z < _spaceSplitSize);
			
			binsX[centerInSplitCoordinates.x].numTriangles += 1;
			binsY[centerInSplitCoordinates.y].numTriangles += 1;
			binsZ[centerInSplitCoordinates.z].numTriangles += 1;
		}
	};
	
	for (auto triIndex : node->triangles)
		placePointToBins(triangleCentroid(_triangles.at(triIndex)));
	
	vec3 splitSquare;
	splitSquare.x = binHalfSize.x * (bbox.halfSize.y() + bbox.halfSize.z()) + bbox.halfSize.y() * bbox.halfSize.z();
	splitSquare.y = binHalfSize.y * (bbox.halfSize.x() + bbox.halfSize.z()) + bbox.halfSize.x() * bbox.halfSize.z();
	splitSquare.z = binHalfSize.z * (bbox.halfSize.x() + bbox.halfSize.y()) + bbox.halfSize.y() * bbox.halfSize.x();
	
	std::vector<Split> splitCosts(_spaceSplitSize);
	splitCosts.back().cost = vec3(std::numeric_limits<float>::max());
	
	vec3 fTotalTriangles(static_cast<float>(node->triangles.size()));
	
	for (int i = 0; i + 1 < _spaceSplitSize; ++i)
	{
		vec3 numTrianglesFromLeft(0.0f);
		float numSplitsFromLeft = float(i + 1);
		
		vec3 numTrianglesFromRight(0.0f);
		float numSplitsFromRight = float(_spaceSplitSize - i - 1);
		
		for (int j = 0; j < _spaceSplitSize; ++j)
		{
			vec3 fCurrentTriangles(float(binsX[j].numTriangles), float(binsY[j].numTriangles), float(binsZ[j].numTriangles));
			((j <= i) ? numTrianglesFromLeft : numTrianglesFromRight) += fCurrentTriangles / fTotalTriangles;
		}
		
		splitCosts[i].cost = splitSquare *
			(numTrianglesFromLeft * numSplitsFromLeft + numTrianglesFromRight * numSplitsFromRight);
		
		splitCosts[i].index = i;
	}
	
	std::vector<Split> minSplits(3);
	std::sort(splitCosts.begin(), splitCosts.end(), [](const Split& l, const Split& r)
		{ return l.cost.x < r.cost.x; });
	minSplits[0] = splitCosts.front();
	minSplits[0].cost = vec3(minSplits[0].cost.x);
	minSplits[0].axis = 0;

	std::sort(splitCosts.begin(), splitCosts.end(), [](const Split& l, const Split& r)
		{ return l.cost.y < r.cost.y; });
	minSplits[1] = splitCosts.front();
	minSplits[1].cost = vec3(minSplits[1].cost.y);
	minSplits[1].axis = 1;

	std::sort(splitCosts.begin(), splitCosts.end(), [](const Split& l, const Split& r)
		{ return l.cost.z < r.cost.z; });
	minSplits[2] = splitCosts.front();
	minSplits[2].cost = vec3(minSplits[2].cost.z);
	minSplits[2].axis = 2;
	
	std::sort(minSplits.begin(), minSplits.end(), [](const Split& l, const Split& r)
		{ return l.cost.x < r.cost.x; });
	
	int axis = minSplits.front().axis;
	int index = minSplits.front().index;
	float splitPlane = bins[axis]->at(index).center[axis] + binHalfSize[axis];
	
	buildSplitBoxesUsingAxisAndPosition(node, axis, splitPlane);
	distributeTrianglesToChildren(node);
	splitNodeUsingBins(node->left, depth + 1);
	splitNodeUsingBins(node->right, depth + 1);
}

void KDTree::buildSplitBoxesUsingAxisAndPosition(Node* node, int axis, float position)
{
	vec4simd lowerCorner = node->boundingBox.center - node->boundingBox.halfSize;
	vec4simd upperCorner = node->boundingBox.center + node->boundingBox.halfSize;
	
	vec4 axisScale4(1.0f);
	axisScale4[axis] = 0.0f;
	
	vec4 posScale4(0.0f);
	posScale4[axis] = 1.0f;
	
	vec4simd axisScale(axisScale4);
	vec4simd posScale(posScale4);
	
	vec4simd middlePoint = lowerCorner * axisScale + posScale * position;
	vec4simd leftSize = (middlePoint - lowerCorner) * posScale * 0.5f;
	vec4simd rightSize = (upperCorner - middlePoint) * posScale * 0.5f;
	
	node->splitAxis = axis;
	node->splitDistance = position;
	
	node->left = sharedObjectFactory().createObject<Node>();
	node->left->boundingBox.center = node->boundingBox.center * axisScale + posScale * (middlePoint - leftSize);
	node->left->boundingBox.halfSize = node->boundingBox.halfSize * axisScale + posScale * leftSize;
	
	node->right = sharedObjectFactory().createObject<Node>();
	node->right->boundingBox.center = node->boundingBox.center * axisScale + posScale * (middlePoint + rightSize);
	node->right->boundingBox.halfSize = node->boundingBox.halfSize * axisScale + posScale * rightSize;
	
	node->containsSubNodes = 1;
}

void KDTree::distributeTrianglesToChildren(Node* node)
{
	ET_ALIGNED(16) vec4 minVertex;
	ET_ALIGNED(16) vec4 maxVertex;
	for (auto triIndex : node->triangles)
	{
		const auto& tri = _triangles.at(triIndex);
		tri.minVertex().loadToFloats(minVertex.data());
		tri.maxVertex().loadToFloats(maxVertex.data());
		
		if (minVertex[node->splitAxis] > node->splitDistance + rt::Constants::epsilon)
		{
			node->right->triangles.push_back(triIndex);
		}
		else if (maxVertex[node->splitAxis] < node->splitDistance - rt::Constants::epsilon)
		{
			node->left->triangles.push_back(triIndex);
		}
		else
		{
			node->left->triangles.push_back(triIndex);
			node->right->triangles.push_back(triIndex);
		}
	}
}

void KDTree::cleanUp()
{
	cleanUpRecursively(_root);
	_triangles.clear();
	_root = nullptr;
	_maxDepth = 0;
	_spaceSplitSize = 0;
}

void KDTree::cleanUpRecursively(Node* node)
{
	if (node == nullptr) return;
	
	cleanUpRecursively(node->left);
	cleanUpRecursively(node->right);
	sharedObjectFactory().deleteObject(node);
}

void KDTree::splitNodeUsingSortedArray(Node* node, size_t depth)
{
	if ((depth > _maxDepth) || (node->triangles.size() <= _minTrianglesToSubdivide))
		return;
	
	int currentAxis = 0;
		
	auto estimateCostAtSplit = [node, &currentAxis](float splitPlane, size_t leftTriangles,
		size_t rightTriangles) -> float
	{
		ET_ASSERT((leftTriangles + rightTriangles) == node->triangles.size());
		
		const auto& bbox = node->boundingBox;
		
		const vec4& minVertex = bbox.minVertex().toVec4();
		if (splitPlane <= minVertex[currentAxis] + std::numeric_limits<float>::epsilon())
			return std::numeric_limits<float>::max();

		const vec4& maxVertex = bbox.maxVertex().toVec4();
		if (splitPlane >= maxVertex[currentAxis] - std::numeric_limits<float>::epsilon())
			return std::numeric_limits<float>::max();

		vec4 axisScale(1.0f);
		vec4 axisOffset(0.0f);
		axisScale[currentAxis] = 0.0f;
		axisOffset[currentAxis] = splitPlane;
		rt::BoundingBox leftBox(bbox.minVertex(), bbox.maxVertex() * vec4simd(axisScale) + vec4simd(axisOffset), 0);
		rt::BoundingBox rightBox(bbox.minVertex() * vec4simd(axisScale) + vec4simd(axisOffset), bbox.maxVertex(), 0);
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
	for (size_t triIndex : node->triangles)
	{
		const auto& tri = _triangles.at(triIndex);
		auto minVertex = tri.v[0].minWith(tri.v[1].minWith(tri.v[2]));
		auto maxVertex = tri.v[0].maxWith(tri.v[1].maxWith(tri.v[2]));
		minPoints.push_back(minVertex.xyz());
		maxPoints.push_back(maxVertex.xyz());
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
			float costMin = estimateCostAtSplit(minPoints.at(i)[currentAxis], i, numElements - i);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMin))
			{
				splitPosition[currentAxis] = minPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
		
		for (int i = numElements - 2; i > 0; --i)
		{
			float costMax = estimateCostAtSplit(maxPoints.at(i)[currentAxis], i, numElements - i);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMax))
			{
				splitPosition[currentAxis] = maxPoints.at(i)[currentAxis];
				splitFound = true;
			}
		}
	}
	
	if (!splitFound)
		return;
	
	float targetValue = etMin(splitCost.x, etMin(splitCost.y, splitCost.z));
	for (int axis = 0; axis < 3; ++axis)
	{
		if (splitCost[axis] == targetValue)
		{
			currentAxis = axis;
			break;
		}
	}
	
	buildSplitBoxesUsingAxisAndPosition(node, currentAxis, splitPosition[currentAxis]);
	distributeTrianglesToChildren(node);
	splitNodeUsingSortedArray(node->left, depth + 1);
	splitNodeUsingSortedArray(node->right, depth + 1);
}

void KDTree::printStructure()
{
	printStructure(_root, std::string());	
}

void KDTree::printStructure(Node* node, const std::string& tag)
{
	if (node == nullptr) return;
	
	const char* axis[] = { "X", "Y", "Z" };
	if (node->containsSubNodes)
	{
		log::info("%s %llu tris, split [%s, %.3f]", tag.c_str(),
			uint64_t(node->triangles.size()), axis[node->splitAxis], node->splitDistance);
	}
	else
	{
		log::info("%s %llu tris", tag.c_str(), uint64_t(node->triangles.size()));
	}
	printStructure(node->left, tag + "--|");
	printStructure(node->right, tag + "--|");
}

float KDTree::findIntersectionInNode(const rt::Ray& ray, KDTree::Node* node, TraverseResult& result)
{
	result.triangleIndex = InvalidIndex;
	float minDistance = std::numeric_limits<float>::max();
	
	auto trianglesData = _triangles.data();
	size_t* triangleIndex = node->triangles.data();
	size_t* lastIndex = node->triangles.data() + node->triangles.size();
	while (triangleIndex < lastIndex)
	{
		auto index = *triangleIndex++;
		
		vec4simd ip;
		vec4simd bc;
		float distance = 0.0f;
		if (rt::rayTriangle(ray, *(trianglesData + index), ip, bc, distance) && (distance < minDistance))
		{
			result.intersectionPoint = ip;
			result.intersectionPointBarycentric = bc;
			result.triangleIndex = index;
			minDistance = distance;
		}
	}
	
	return (result.triangleIndex == InvalidIndex) ? std::numeric_limits<float>::max() : minDistance;
}

bool KDTree::findIntersection(const rt::Ray& ray, TraverseResult& result, KDTree::Node* node)
{
	result.triangleIndex = InvalidIndex;
	float minDistance = std::numeric_limits<float>::max();
	
	size_t* triangleIndex = node->triangles.data();
	for (size_t i = 0, e = node->triangles.size(); i < e; ++i, ++triangleIndex)
	{
		vec4simd ip;
		vec4simd bc;
		float distance;
		size_t index = *triangleIndex;
		if (rt::rayTriangle(ray, _triangles.at(index), ip, bc, distance))
		{
			if (distance < minDistance)
			{
				result.intersectionPoint = ip;
				result.intersectionPointBarycentric = bc;
				result.triangleIndex = index;
				minDistance = distance;
			}
		}
	}
	
	return (result.triangleIndex != InvalidIndex);
}

const rt::Triangle& KDTree::triangleAtIndex(size_t i) const
{
	return _triangles.at(i);
}

struct TraverseEntry
{
	KDTree::Node* farNode;
	float farTime;
	
	TraverseEntry() { }
	TraverseEntry(KDTree::Node* n, float t) :
		farNode(n), farTime(t) { }
};

struct FastTraverseStack
{
public:
	enum : size_t
	{
		MaxElements = 128,
		MaxElementsPlusOne = MaxElements + 1,
	};
	
public:
	void emplace(KDTree::Node* farNode, float farTime)
	{
		ET_ASSERT(size < MaxElements);
		entries[size].farNode = farNode;
		entries[size].farTime = farTime;
		++size;
	}
	
	bool empty() const
		{ return size == 0; }
	
	const TraverseEntry& top() const
		{ ET_ASSERT(size < MaxElementsPlusOne); return entries[size - 1]; }
	
	void pop()
		{ ET_ASSERT(size > 0); --size; }
	
private:
	TraverseEntry entries[MaxElements];
	size_t size = 0;
};

KDTree::TraverseResult KDTree::traverse(const rt::Ray& r)
{
	KDTree::TraverseResult result;
	
	float tNear = 0.0f;
	float tFar = 0.0f;
	
	if (!rt::rayToBoundingBox(r, _root->boundingBox, tNear, tFar))
		return result;
	
	if (tNear < 0.0f)
		tNear = 0.0f;
	
	Node* currentNode = _root;
	
	ET_ALIGNED(16) vec4 origin;
	ET_ALIGNED(16) vec4 direction;
	r.origin.loadToFloats(origin.data());
	r.direction.loadToFloats(direction.data());
	
	FastTraverseStack traverseStack;
	for (;;)
	{
		while (currentNode->containsSubNodes)
		{
			size_t axis = currentNode->splitAxis;
			size_t side = static_cast<size_t>(direction[axis] < 0.0f);
			
			float tSplit = (currentNode->splitDistance - origin[axis]) / direction[axis];
			
			if (tSplit <= tNear - rt::Constants::epsilon)
			{
				currentNode = currentNode->subNodes[1 - side];
			}
			else if (tSplit >= tFar + rt::Constants::epsilon)
			{
				currentNode = currentNode->subNodes[side];
			}
			else
			{
				traverseStack.emplace(currentNode->subNodes[1 - side], tFar);
				currentNode = currentNode->subNodes[side];
				tFar = tSplit;
			}
		}
		
		if (!currentNode->triangles.empty())
		{
			float tHit = findIntersectionInNode(r, currentNode, result);
			if (tHit <= tFar + rt::Constants::epsilon)
				return result;
		}
		
		if (traverseStack.empty())
		{
			result.triangleIndex = InvalidIndex;
			return result;
		}
		
		currentNode = traverseStack.top().farNode;
		tNear = tFar;
		tFar = traverseStack.top().farTime;
		
		traverseStack.pop();
	}
	
	return result;
}
