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

KDTree::Node* KDTree::buildRootNode(const std::vector<rt::Triangle>& triangles)
{
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
	
	KDTree::Node* result = sharedObjectFactory().createObject<KDTree::Node>();
	result->depth = 0;
	result->boundingBox = rt::BoundingBox(center, halfSize);
	result->triangles.reserve(triangles.size());
	for (size_t i = 0, e = triangles.size(); i < e; ++i)
		result->triangles.push_back(i);
	
	return result;
}

void KDTree::build(const std::vector<rt::Triangle>& triangles, size_t maxDepth, int splits)
{
	cleanUp();
	_root = buildRootNode(triangles);
	_maxDepth = maxDepth;
	_spaceSplitSize = splits;
	splitNodeUsingSortedArray(_root, triangles);
	printStructure(_root, std::string());
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

void KDTree::splitNodeUsingBins(Node* node, const std::vector<rt::Triangle>& triangles, size_t depth)
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
		ET_ASSERT(rt::pointInsideBoundingBox(vec4simd(point, 1.0f), bbox));
		
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
	};
	
	for (auto triIndex : node->triangles)
	{
		const auto& tri = triangles.at(triIndex);
		/*
		{
			log::info("\nTriangle V0: (%.3f, %.3f, %.3f)\n"
					  "Triangle V1: (%.3f, %.3f, %.3f)\n"
					  "Triangle V2: (%.3f, %.3f, %.3f)\n\n",
				tri.v[0].x(), tri.v[0].y(), tri.v[0].z(),
				tri.v[1].x(), tri.v[1].y(), tri.v[1].z(),
				tri.v[2].x(), tri.v[2].y(), tri.v[2].z());
		}
		// */
		placePointToBins(triangleCentroid(tri));
	}
	
	vec3 splitSquare;
	splitSquare.x = 4.0f * (binHalfSize.x * (bbox.halfSize.y() + bbox.halfSize.z()) +
		bbox.halfSize.y() * bbox.halfSize.z());
	splitSquare.y = 4.0f * (binHalfSize.y * (bbox.halfSize.x() + bbox.halfSize.z()) +
		bbox.halfSize.x() * bbox.halfSize.z());
	splitSquare.z = 4.0f * (binHalfSize.z * (bbox.halfSize.x() + bbox.halfSize.y()) +
		bbox.halfSize.y() * bbox.halfSize.x());
	
	std::vector<Split> splitCosts(_spaceSplitSize);
	
	for (int i = 0; i < _spaceSplitSize; ++i)
	{
		vec3 numTrianglesFromLeft(0.0f);
		float numSplitsFromLeft = float(i + 1);
		
		vec3 numTrianglesFromRight(0.0f);
		float numSplitsFromRight = float(_spaceSplitSize - i - 1);
		
		for (int j = 0; j < _spaceSplitSize; ++j)
		{
			((j <= i) ? numTrianglesFromLeft : numTrianglesFromRight) +=
				vec3(float(binsX[j].numTriangles), float(binsY[j].numTriangles), float(binsZ[j].numTriangles));
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
	minSplits[2].cost = vec3(minSplits[2].cost.x);
	minSplits[2].axis = 2;
	
	std::sort(minSplits.begin(), minSplits.end(), [](const Split& l, const Split& r)
		{ return l.cost.x < r.cost.x; });
	
	int axis = minSplits.front().axis;
	int index = minSplits.front().index;
	float splitPlane = bins[axis]->at(index).center[axis] + binHalfSize[axis];
	
	buildSplitBoxesUsingAxisAndPosition(node, axis, splitPlane);
	distributeTrianglesToChildren(node, triangles);

	const char* axisNames[3] = { "X", "Y", "Z" };
	
	log::info("Node with depth (%llu), containing %llu triangles, split along %s axis at position %.4f:",
		static_cast<uint64_t>(node->depth), static_cast<uint64_t>(node->triangles.size()), axisNames[axis], splitPlane);
	log::info("Left, containing %llu triangles", static_cast<uint64_t>(node->left->triangles.size()));
	log::info("Right, containing %llu triangles", static_cast<uint64_t>(node->right->triangles.size()));
	
	splitNodeUsingBins(node->left, triangles, depth + 1);
	splitNodeUsingBins(node->right, triangles, depth + 1);
	
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
	
	node->left = sharedObjectFactory().createObject<Node>();
	node->left->depth = node->depth + 1;
	node->left->boundingBox.center = node->boundingBox.center * axisScale +
		posScale * (middlePoint - leftSize);
	node->left->boundingBox.halfSize = node->boundingBox.halfSize * axisScale + posScale * leftSize;
	
	node->right = sharedObjectFactory().createObject<Node>();
	node->right->depth = node->depth + 1;
	node->right->boundingBox.center = node->boundingBox.center * axisScale +
		posScale * (middlePoint + rightSize);
	node->right->boundingBox.halfSize = node->boundingBox.halfSize * axisScale + posScale * rightSize;
}

void KDTree::distributeTrianglesToChildren(Node* node, const std::vector<rt::Triangle>& triangles)
{
	for (auto triIndex : node->triangles)
	{
		vec4simd c(triangleCentroid(triangles.at(triIndex)), 0.0f);
		
		bool distributed = false;
		
		if (rt::pointInsideBoundingBox(c, node->left->boundingBox))
		{
			distributed = true;
			node->left->triangles.push_back(triIndex);
		}
		
		if (rt::pointInsideBoundingBox(c, node->right->boundingBox))
		{
			distributed = true;
			node->right->triangles.push_back(triIndex);
		}
		
		if (!distributed)
		{
			node->left->triangles.push_back(triIndex);
			node->right->triangles.push_back(triIndex);
		}
	}
}

void KDTree::cleanUp()
{
	cleanUpRecursively(_root);
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

KDTree::TraverseResult KDTree::traverse(const rt::Ray& ray)
{
	KDTree::TraverseResult result;
	traverseRecursive(ray, _root, result);
	return result;
}

void KDTree::traverseRecursive(const rt::Ray& ray, Node* node, TraverseResult& result)
{
	if (rt::rayHitsBoundingBox(ray, node->boundingBox))
		result.push(node);
}

void KDTree::splitNodeUsingSortedArray(Node* node, const std::vector<rt::Triangle>& triangles)
{
	if ((node->depth > _maxDepth) || (node->triangles.size() < _minTrianglesToSubdivide))
		return;
	
	int currentAxis = 0;
		
	auto estimateCostAtSplit = [node, &currentAxis](float splitPlane, size_t leftTriangles,
		size_t rightTriangles) -> float
	{
		ET_ASSERT((leftTriangles + rightTriangles) == node->triangles.size());
		
		const auto& bbox = node->boundingBox;
		
		const vec4& minVertex = bbox.minVertex().toVec4();
		if (splitPlane - std::numeric_limits<float>::epsilon() <= minVertex[currentAxis])
			return std::numeric_limits<float>::max();

		const vec4& maxVertex = bbox.maxVertex().toVec4();
		if (splitPlane + std::numeric_limits<float>::epsilon() >= maxVertex[currentAxis])
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
		
		/*
		log::info("Testing axis %d, position: %01.2f\tT: (%llu <|> %llu)\tS: (%02.2f <|> %02.2f) --> %.3f + %.3f = %.3f",
			currentAxis, splitPlane, uint64_t(leftTriangles), uint64_t(rightTriangles),
			leftSquare, rightSquare, costLeft, costRight, costLeft + costRight);
		// */
		
		return costLeft + costRight;
	};
	
	auto compareAndAssignMinimum = [](float& minCost, float cost) -> bool
	{
		bool result = cost <= minCost;
		if (result)
			minCost = cost;
		return result;
	};
	
	std::vector<vec3> minPoints;
	std::vector<vec3> maxPoints;
	for (size_t triIndex : node->triangles)
	{
		const auto& tri = triangles.at(triIndex);
		/*
		log::info("triIndex: %llu, tri:\n(%.3f %.3f %.3f)\n(%.3f %.3f %.3f)\n(%.3f %.3f %.3f)",
			uint64_t(triIndex),
			tri.v[0].x(), tri.v[0].y(), tri.v[0].z(),
			tri.v[1].x(), tri.v[1].y(), tri.v[1].z(),
			tri.v[2].x(), tri.v[2].y(), tri.v[2].z());
		// */
		auto minVertex = tri.v[0].minWith(tri.v[1].minWith(tri.v[2]));
		auto maxVertex = tri.v[0].maxWith(tri.v[1].maxWith(tri.v[2]));
		minPoints.push_back(minVertex.xyz());
		maxPoints.push_back(maxVertex.xyz());
	}
	
	vec3 splitPosition = minPoints.at(minPoints.size() / 2);
	vec3 splitCost(std::numeric_limits<float>::max());
	
	int numElements = static_cast<int>(minPoints.size());
	
	for (int axis = 0; axis < 3; ++axis)
	{
		currentAxis = axis;
		std::sort(minPoints.begin(), minPoints.end(), [&currentAxis](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });
		std::sort(maxPoints.begin(), maxPoints.end(), [&currentAxis](const vec3& l, const vec3& r)
			{ return l[currentAxis] < r[currentAxis]; });
		
		for (int i = 1; i + 1 < numElements; ++i)
		{
			float costMin = estimateCostAtSplit(minPoints.at(i)[currentAxis], i, numElements - i);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMin))
				splitPosition[currentAxis] = minPoints.at(i)[currentAxis];
		}
		
		for (int i = numElements - 2; i > 0; --i)
		{
			float costMax = estimateCostAtSplit(maxPoints.at(i)[currentAxis], i, numElements - i);
			if (compareAndAssignMinimum(splitCost[currentAxis], costMax))
				splitPosition[currentAxis] = maxPoints.at(i)[currentAxis];
		}
	}
	
	log::info("Split values and costs: (%.3f~%.3f, %.3f~%.3f, %.3f~%.3f)", splitPosition.x, splitCost.x,
		splitPosition.y, splitCost.y, splitPosition.z, splitCost.z);
	
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
	
	const float epsilon = std::numeric_limits<float>::epsilon();
	for (size_t triIndex : node->triangles)
	{
		const auto& tri = triangles.at(triIndex);
		float minPoint = tri.v[0].minWith(tri.v[1].minWith(tri.v[2])).toVec4()[currentAxis];
		float maxPoint = tri.v[0].maxWith(tri.v[1].maxWith(tri.v[2])).toVec4()[currentAxis];
		if (maxPoint <= splitPosition[currentAxis] + epsilon)
		{
			node->left->triangles.push_back(triIndex);
		}
		else if (minPoint >= splitPosition[currentAxis] - epsilon)
		{
			node->right->triangles.push_back(triIndex);
		}
		else
		{
			node->left->triangles.push_back(triIndex);
			node->right->triangles.push_back(triIndex);
		}
	}
	
	const char* axisNames[3] = { "X", "Y", "Z" };
	
	log::info("Split node (%llu, %llu triangles) along %s axis in at %f, child nodes have %llu and %llu triangles",
		uint64_t(node->depth), uint64_t(node->triangles.size()), axisNames[currentAxis], splitPosition[currentAxis],
		uint64_t(node->left->triangles.size()), uint64_t(node->right->triangles.size()));
	
	/*
	printf("Left: (");
	for (size_t t : node->left->triangles)
		printf("%zu ", t);
	printf(")\n");
	printf("Right: (");
	for (size_t t : node->right->triangles)
		printf("%zu ", t);
	printf(")\n");
	*/
	
	splitNodeUsingSortedArray(node->left, triangles);
	splitNodeUsingSortedArray(node->right, triangles);
}

void KDTree::printStructure(Node* node, const std::string& tag)
{
	if (node == nullptr) return;
	log::info("%s %llu (%llu)", tag.c_str(), uint64_t(node->depth), uint64_t(node->triangles.size()));
	printStructure(node->left, tag + "--|");
	printStructure(node->right, tag + "--|");
}
