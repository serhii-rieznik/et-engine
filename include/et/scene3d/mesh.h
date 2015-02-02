/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vertexarrayobject.h>
#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		class Mesh : public RenderableElement
		{
		public:
			ET_DECLARE_POINTER(Mesh)
			
			static const std::string defaultMeshName;

		public:
			Mesh(const std::string& = defaultMeshName, Element* = nullptr);
			Mesh(const std::string&, const VertexArrayObject&, const Material::Pointer&, uint32_t, uint32_t, Element* = nullptr);

			ElementType type() const 
				{ return ElementType_Mesh; }

			Mesh* duplicate();

			VertexArrayObject& vertexArrayObject();
			const VertexArrayObject& vertexArrayObject() const;

			VertexBuffer::Pointer& vertexBuffer();
			const VertexBuffer::Pointer& vertexBuffer() const;

			IndexBuffer& indexBuffer();
			const IndexBuffer& indexBuffer() const;

			uint32_t startIndex() const;
			void setStartIndex(uint32_t index);
			
			uint32_t numIndexes() const;
			virtual void setNumIndexes(uint32_t num);

			void setVertexBuffer(VertexBuffer::Pointer vb);
			void setIndexBuffer(IndexBuffer ib);
			void setVertexArrayObject(VertexArrayObject vao);

			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);

			void cleanupLodChildren();
			void attachLod(uint32_t level, Mesh::Pointer mesh);

			void setLod(uint32_t level);
			
			const std::string& vaoName() const
				{ return _vaoName; }
			
			const std::string& vbName() const
				{ return _vbName; }
			
			const std::string& ibName() const
				{ return _ibName; }

		private:
			Mesh* currentLod();
			const Mesh* currentLod() const;

		private:
			VertexArrayObject _vao;
			std::map<uint32_t, Mesh::Pointer> _lods;
			uint32_t _startIndex = 0;
			uint32_t _numIndexes = 0;
			uint32_t _selectedLod = 0;
			std::string _vaoName;
			std::string _vbName;
			std::string _ibName;
		};
	}
}
