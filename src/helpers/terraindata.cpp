/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/collision.h>
#include <et/timers/intervaltimer.h>
#include <et/rendering/primitives.h>
#include <et/helpers/terraindata.h>

using namespace et;

TerrainData::TerrainData(TerrainDataDelegate* aDelegate) : _delegate(aDelegate)
{
}

TerrainData::TerrainData(TerrainDataDelegate* aDelegate, std::istream& stream, const vec2i& dimension, Format format) : _delegate(aDelegate)
{
	loadFromStream(stream, dimension, format);
}

void TerrainData::loadFromStream(std::istream& stream, const vec2i& dimension, Format format)
{
	_dimension = dimension;
	_dimensionf.x = static_cast<float>(dimension.x - 1);
	_dimensionf.y = static_cast<float>(dimension.y - 1);

	int size = dimension.square();

	FloatDataStorage heightmap(size);

	if (format == TerrainData::Format_8bit)
	{
		DataStorage<unsigned char> buffer(size);
		stream.read(buffer.binary(), size);
		for (int i = 0; i < size; ++i)
			heightmap.push_back(static_cast<float>(buffer[i]) / 255.0f);
	}
	else
	{
		DataStorage<unsigned short> buffer(size);
		stream.read(buffer.binary(), size * sizeof(unsigned short));
		for (int i = 0; i < size; ++i)
			heightmap.push_back(static_cast<float>(buffer[i]) / 65535.0f);
	}

	generateVertexData(heightmap);
}

void TerrainData::generateVertexData(const FloatDataStorage& hm)
{
	vec2i gridPos(0);

	float x = 0.0f;
	float z = 0.0f;

	VertexDeclaration decl(true);
	decl.push_back(VertexAttributeUsage::Position, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::TexCoord0, DataType::Vec2);
	decl.push_back(VertexAttributeUsage::Tangent, DataType::Vec3);

	_vertexData = VertexArray::Pointer::create(decl, _dimension.square());

	RawDataAcessor<vec3> pos = _vertexData->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);
	RawDataAcessor<vec3> nrm = _vertexData->chunk(VertexAttributeUsage::Normal).accessData<vec3>(0);
	RawDataAcessor<vec3> tan = _vertexData->chunk(VertexAttributeUsage::Tangent).accessData<vec3>(0);
	RawDataAcessor<vec2> texc = _vertexData->chunk(VertexAttributeUsage::TexCoord0).accessData<vec2>(0);

	_positions.resize(hm.size());
	_positions.setOffset(0);

	bool firstMinVertex = true;
	bool firstMaxVertex = true;
	for (uint32_t i = 0; i < hm.size(); ++i)
	{
		float dx = x / static_cast<float>(_dimension.x - 1);
		float dz = z / static_cast<float>(_dimension.y - 1);

		pos[i] = vec3(dx, hm[i], dz);
		texc[i] = vec2(dx, dz);
		if (_delegate)
		{
			pos[i] = _delegate->processTerrainVertex(this, pos[i], gridPos);
			texc[i] = _delegate->processTerrainTexCoord(this, texc[i], gridPos);
		}
		_positions.push_back(pos[i]);

		_minVertex = firstMinVertex ? pos[i] : minv(pos[i], _minVertex);
		_maxVertex = firstMaxVertex ? pos[i] : maxv(pos[i], _maxVertex);

		firstMinVertex = false;
		firstMaxVertex = false;

		x += 1.0f;
		++gridPos.x;
		if (gridPos.x >= _dimension.x)
		{
			gridPos.x = 0;
			x = 0.0f;
			z += 1.0f;
			++gridPos.y;
		}

		nrm[i] = vec3(0.0f);
		tan[i] = vec3(0.0f);
	}

	_bounds = BoundingBox(0.5f * (_minVertex + _maxVertex), _maxVertex - _minVertex);

	uint32_t numTriangles = primitives::indexCountForRegularMesh(_dimension, PrimitiveType::Triangles);
	IndexArray::Pointer tempIB = IndexArray::Pointer::create(IndexArrayFormat::Format_32bit, numTriangles, PrimitiveType::Triangles);
	
	primitives::buildTrianglesIndexes(tempIB, _dimension, 0, 0);
	primitives::calculateNormals(_vertexData, tempIB, 0, numTriangles);
	primitives::calculateTangents(_vertexData, tempIB, 0, numTriangles);

	_normals.resize(hm.size());
	_normals.setOffset(0);
	
	for (uint32_t i = 0; i < hm.size(); ++i)
		_normals.push_back(nrm[i]);
}

vec3 TerrainData::normalAtNormalizedPoint(const vec2& normalized) const
{
	vec2i p00(static_cast<int>(normalized.x), static_cast<int>(normalized.y));
	vec2i p11(std::min(p00.x + 1, _dimension.x - 1), std::min(p00.y + 1, _dimension.y - 1));

	vec2 dp(normalized.x - static_cast<float>(p00.x), normalized.y - static_cast<float>(p00.y));

	vec3 n00 = normalAtXZ(p00);
	vec3 n11 = normalAtXZ(p00);

	if (dp.y > dp.x)
	{
		vec3 n10 = normalAtXZ(vec2i(p00.x, p11.y));
		return normalize( n10 + (1.0f - dp.y) * (n00 - n10) + dp.x * (n11 - n10) );
	}
	else
	{
		vec3 n01 = normalAtXZ(vec2i(p11.x, p00.y));
		return normalize( n01 + dp.y * (n11 - n01) + (1.0f - dp.x) * (n00 - n01) );
	}
}

vec3 TerrainData::normalAtPoint(const vec3& pt) const
{
	return normalAtNormalizedPoint(normalizePoint(pt));
}

float TerrainData::heightAtNormalizedPoint(const vec2& normalized) const
{
	vec2i p00(static_cast<int>(normalized.x), static_cast<int>(normalized.y));
	vec2i p11(std::min(p00.x + 1, _dimension.x - 1), std::min(p00.y + 1, _dimension.y - 1));

	vec2 dp(normalized.x - static_cast<float>(p00.x), normalized.y - static_cast<float>(p00.y));

	float h00 = heightAtXZ(p00);
	float h11 = heightAtXZ(p00);

	if (dp.y > dp.x)
	{
		float h10 = heightAtXZ(vec2i(p00.x, p11.y));
		return h10 + (1.0f - dp.y) * (h00 - h10) + dp.x * (h11 - h10);
	}
	else
	{
		float h01 = heightAtXZ(vec2i(p11.x, p00.y));
		return h01 + dp.y * (h11 - h01) + (1.0f - dp.x) * (h00 - h01);
	}
}

float TerrainData::heightAtPoint(const vec3& pt) const
{
	return heightAtNormalizedPoint(normalizePoint(pt));
}

const vec3& TerrainData::normalAtIndex(int index) const
{
	return _normals[index];
}

const vec3& TerrainData::normalAtXZ(const vec2i& xy) const
{
	return _normals[xy.x + _dimension.x * xy.y];
}

float TerrainData::heightAtIndex(int index) const
{
	return _positions[index].y;
}

float TerrainData::heightAtXZ(const vec2i& xy) const
{
	return _positions[xy.x + _dimension.x * xy.y].y;
}

const vec3& TerrainData::positionAtIndex(int index) const
{
	return _positions[index];
}

const vec3& TerrainData::positionAtXZ(const vec2i& xy) const
{
	return _positions[xy.x + _dimension.x * xy.y];
}

vec2 TerrainData::normalizePoint(const vec3& pt) const
{
	float nx = _dimensionf.x * clamp((pt.x - _minVertex.x) / _bounds.halfDimension.x, 0.0f, 1.0f);
	float nz = _dimensionf.y * clamp((pt.z - _minVertex.z) / _bounds.halfDimension.z, 0.0f, 1.0f);
	return vec2(nx, nz);
}

triangle TerrainData::triangleForXZ(const vec2i& pt0, int side) const
{
	vec3 v1 = positionAtXZ(pt0);
	vec3 v2;
	vec3 v3;

	vec2i pt1;
	vec2i pt2 = pt0 + vec2i(1);
	if (pt2.x >= _dimension.x) 
		pt2.x = _dimension.x - 1;
	if (pt2.y >= _dimension.y) 
		pt2.y = _dimension.y - 1;

	if (side == 0)
	{
		pt1 = vec2i(pt0.x, pt0.y + 1);
		if (pt1.y >= _dimension.y) 
			pt1.y = _dimension.y - 1;
		v2 = positionAtXZ(pt1);
		v3 = positionAtXZ(pt2);
	}
	else 
	{
		pt1 = vec2i(pt0.x + 1, pt0.y);
		if (pt1.x >= _dimension.x) 
			pt1.x = _dimension.x - 1;
		v2 = positionAtXZ(pt2);
		v3 = positionAtXZ(pt1);
	}

	return triangle(v1, v2, v3);
}

TerrainContact TerrainData::contactForSphere(const Sphere& s) const
{
	vec3 radiusVector(s.radius(), 0.0f, s.radius());
	vec2 leftTopProj = normalizePoint(s.center() - radiusVector);
	vec2 rightBottomProj = normalizePoint(s.center() + radiusVector);

	vec2i leftTopIndex(static_cast<int>(leftTopProj.x), static_cast<int>(leftTopProj.y));
	vec2i rightBottomIndex(static_cast<int>(rightBottomProj.x), static_cast<int>(rightBottomProj.y));

	TerrainContact t;

	float squareRadius = sqr(s.radius());
	int numPoints = 0;

	for (int y = leftTopIndex.y; y <= rightBottomIndex.y; ++y)
	{
		for (int x = leftTopIndex.x; x <= rightBottomIndex.x; ++x)
		{
			triangle t1 = triangleForXZ(vec2i(x, y), 0);
			triangle t2 = triangleForXZ(vec2i(x, y), 1);
			vec3 projCtoP1 = plane(t1).projectionOfPoint(s.center());
			vec3 projCtoP2 = plane(t2).projectionOfPoint(s.center());
			float d1 = (projCtoP1 - s.center()).dotSelf();
			float d2 = (projCtoP1 - s.center()).dotSelf();

			if ((d1 <= squareRadius) && (pointInsideTriangle(projCtoP1, t1)))
			{
				++numPoints;
				t.normal += t1.normalizedNormal();
				t.point += projCtoP1;
			}

			if ((d2 <= squareRadius) && (pointInsideTriangle(projCtoP2, t2)))
			{
				++numPoints;
				t.normal += t2.normalizedNormal();
				t.point += projCtoP2;
			}

		}
	}

	t.contacted = numPoints > 0;
	if (t.contacted)
	{
		t.point /= static_cast<float>(numPoints);
		t.normal = normalize(t.normal);
	}

	return t;
}

void TerrainData::gatherContactsForSphere(const Sphere& s, TerrainDataDelegate* contactDelegate) const
{
	vec3 radiusVector(s.radius(), 0.0f, s.radius());
	vec2 leftTopProj = normalizePoint(s.center() - radiusVector);
	vec2 rightBottomProj = normalizePoint(s.center() + radiusVector);
	vec2i leftTopIndex(static_cast<int>(leftTopProj.x), static_cast<int>(leftTopProj.y));
	vec2i rightBottomIndex(static_cast<int>(rightBottomProj.x), static_cast<int>(rightBottomProj.y));

	float squareRadius = sqr(s.radius());

	for (int y = leftTopIndex.y; y <= rightBottomIndex.y; ++y)
	{
		for (int x = leftTopIndex.x; x <= rightBottomIndex.x; ++x)
		{
			triangle t1 = triangleForXZ(vec2i(x, y), 0);
			vec3 projCtoP1 = plane(t1).projectionOfPoint(s.center());
			float d1 = (projCtoP1 - s.center()).dotSelf();
			if ((d1 <= squareRadius) && (pointInsideTriangle(projCtoP1, t1)))
				contactDelegate->terrainDataDidFindContact(TerrainContact(projCtoP1, normalize(t1.normalizedNormal()), true));

			triangle t2 = triangleForXZ(vec2i(x, y), 1);
			vec3 projCtoP2 = plane(t2).projectionOfPoint(s.center());
			float d2 = (projCtoP1 - s.center()).dotSelf();
			if ((d2 <= squareRadius) && (pointInsideTriangle(projCtoP2, t2)))
				contactDelegate->terrainDataDidFindContact(TerrainContact(projCtoP2, normalize(t2.normalizedNormal()), true));
		}
	}
}
