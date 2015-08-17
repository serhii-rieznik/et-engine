/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>

namespace et
{
	class KDTree
	{
	public:
		struct Node
		{
			Node* left = nullptr;
			Node* right = nullptr;
			std::vector<size_t> triangles;
		};
		
	public:
		void build(const std::vector<triangleEx>&);
		void cleanUp();
		
	private:
		void cleanUpRecursively(Node*);
		
	private:
		Node* _root = nullptr;
	};
}
