/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/collision/collision.h>
#include <et/vertexbuffer/vertexarray.h>

namespace et
{
	class TerrainData;

	struct TerrainContact
	{
		vec3 point;
		vec3 normal;
		bool contacted;

		TerrainContact() : contacted(false) { }
		TerrainContact(const vec3& pt, const vec3& n, bool c) : point(pt), normal(n), contacted(c) { }
	};

	class TerrainDataDelegate
	{
	public:
		virtual vec3 processTerrainVertex(TerrainData const*, const vec3& input, const vec2i&/* gridPosition */) 
			{ return input; }

		virtual vec2 processTerrainTexCoord(TerrainData const*, const vec2& input, const vec2i&/* gridPosition */) 
			{ return input; }

		virtual void terrainDataDidFindContact(const TerrainContact&)
			{ }
	};

	class TerrainData : public Shared
	{
	public:
		enum Format
		{
			Format_8bit,
			Format_16bit
		};

	public:
		TerrainData(TerrainDataDelegate* aDelegate);
		TerrainData(TerrainDataDelegate* aDelegate, std::istream& stream, const vec2i& dimension, Format format);

		void loadFromStream(std::istream& stream, const vec2i& dimension, Format format);

		inline const vec2i& dimension() const 
			{ return _dimension; }

		const AABB& bounds() const
			{ return _bounds; }

		const VertexArray::Pointer& vertexData() const
			{ return _vertexData; }

		VertexArray::Pointer& vertexData()
			{ return _vertexData; }

		vec3 normalAtPoint(const vec3& pt) const;
		float heightAtPoint(const vec3& pt) const;

		TerrainContact contactForSphere(const Sphere& s) const;
		void gatherContactsForSphere(const Sphere& s, TerrainDataDelegate* contactDelegate) const;

	private:
		void generateVertexData(const FloatDataStorage& hm);

		triangle triangleForXZ(const vec2i& pt, int side) const;

		vec2 normalizePoint(const vec3& pt) const;

		vec3 normalAtNormalizedPoint(const vec2& pt) const;
		const vec3& normalAtIndex(int index) const;
		const vec3& normalAtXZ(const vec2i& xy) const;

		float heightAtNormalizedPoint(const vec2& pt) const;
		float heightAtIndex(int index) const;
		float heightAtXZ(const vec2i& xy) const;

		const vec3& positionAtIndex(int index) const;
		const vec3& positionAtXZ(const vec2i& xy) const;

	private:
		TerrainDataDelegate* _delegate;

		vec2i _dimension;
		vec2 _dimensionf;

		vec3 _minVertex;
		vec3 _maxVertex;
		AABB _bounds;

		DataStorage<vec3> _positions;
		DataStorage<vec3> _normals;

		VertexArray::Pointer _vertexData;
	};

	typedef IntrusivePtr<TerrainData> TerrainDataRef;
}
