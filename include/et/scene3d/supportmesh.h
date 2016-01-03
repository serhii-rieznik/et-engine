/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/mesh.h>

namespace et
{
	namespace s3d
	{
		class SupportMesh : public Mesh
		{
		public:
			ET_DECLARE_POINTER(SupportMesh)

		public:
			SupportMesh(const std::string& = defaultMeshName, BaseElement* = nullptr);
			
			SupportMesh(const std::string&, const VertexArrayObject&, const SceneMaterial::Pointer&,
				uint32_t, uint32_t, const VertexStorage::Pointer&, const IndexArray::Pointer&, BaseElement* = nullptr);

			ElementType type() const 
				{ return ElementType::SupportMesh; }

			SupportMesh* duplicate();
		};
	}
}
