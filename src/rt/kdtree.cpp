/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rt/kdtree.h>

using namespace et;

void KDTree::build(const std::vector<triangleEx>&)
{
	cleanUp();
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
	delete node;
}