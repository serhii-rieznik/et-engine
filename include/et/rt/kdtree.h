/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rt/raytraceobjects.h>

namespace et
{
	class KDTree
	{
	public:
		struct Node
		{
			rt::BoundingBox boundingBox;
			std::vector<size_t> triangles;
			Node* left = nullptr;
			Node* right = nullptr;
		};
		
	public:
		void build(const std::vector<rt::Triangle>&);
		void cleanUp();
		
		Node* traverse(const rt::Ray&);
		
	private:
		void cleanUpRecursively(Node*);
		
	private:
		Node* _root = nullptr;
	};
}
