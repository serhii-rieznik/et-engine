/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/primitives/primitives.h>
#include <et/vertexbuffer/vertexstorage.h>
#include <et/scene3d/supportmesh.h>

using namespace et;
using namespace et::s3d;

SupportMesh::SupportMesh(const std::string& name, Element* parent) :
	Mesh(name, parent), _radius(0.0f) { }

SupportMesh::SupportMesh(const std::string& name, const VertexArrayObject& ib, const Material::Pointer& material,
	uint32_t start, uint32_t num, Element* parent) : Mesh(name, ib, material, start, num, parent),
	_data(num / 3, 0), _radius(0.0f)
{
}

void SupportMesh::setNumIndexes(uint32_t num)
{
	Mesh::setNumIndexes(num);
	_data.fitToSize(num / 3);
}

void SupportMesh::fillCollisionData(const VertexStorage::Pointer& v, const IndexArray::Pointer& indexArray)
{
	const auto pos = v->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, 0);
	
	_data.setOffset(0);
	
	vec3 minOffset;
	vec3 maxOffset;
	float distance = 0.0f;
	uint32_t iStart = startIndex() / 3;
	uint32_t iEnd = iStart + static_cast<uint32_t>(numIndexes()) / 3;
	
	_data.resize(primitives::primitiveCountForIndexCount(numIndexes(), indexArray->primitiveType()));
	
	size_t index = 0;
	for (IndexArray::PrimitiveIterator i = indexArray->primitive(iStart), e = indexArray->primitive(iEnd); i != e; ++i)
	{
		const IndexArray::Primitive& p = *i;
		const vec3& p0 = pos[p[0]];
		const vec3& p1 = pos[p[1]];
		const vec3& p2 = pos[p[2]];
		
		_data.push_back(triangle(p0, p1, p2));
		
		if (index == 0)
		{
			minOffset = minv(p0, minv(p1, p2));
			maxOffset = maxv(p0, maxv(p1, p2));
			distance = etMax(p0.length(), etMax(p1.length(), p2.length()));
			++index;
		}
		else
		{
			minOffset = minv(minOffset, p0);
			minOffset = minv(minOffset, p1);
			minOffset = minv(minOffset, p2);
			maxOffset = maxv(maxOffset, p0);
			maxOffset = maxv(maxOffset, p1);
			maxOffset = maxv(maxOffset, p2);
			distance = etMax(p0.length(), etMax(p1.length(), p2.length()));
		}
	}
	
	_size = maxv(maxOffset - minOffset, vec3(std::numeric_limits<float>::epsilon()));
	_center = 0.5f * (maxOffset + minOffset);
	_radius = distance;
}

void SupportMesh::fillCollisionData(const VertexArray::Pointer& vertexArray, const IndexArray::Pointer& indexArray)
{
	const RawDataAcessor<vec3> pos = vertexArray->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);

	_data.setOffset(0);
	
	vec3 minOffset;
	vec3 maxOffset;
	float distance = 0.0f;
	uint32_t iStart = startIndex() / 3;
	uint32_t iEnd = iStart + static_cast<uint32_t>(numIndexes()) / 3;
	
	_data.resize(primitives::primitiveCountForIndexCount(numIndexes(), indexArray->primitiveType()));
	
	size_t index = 0;
	for (IndexArray::PrimitiveIterator i = indexArray->primitive(iStart), e = indexArray->primitive(iEnd); i != e; ++i)
	{
		const IndexArray::Primitive& p = *i;
		const vec3& p0 = pos[p[0]];
		const vec3& p1 = pos[p[1]];
		const vec3& p2 = pos[p[2]];
		
		_data.push_back(triangle(p0, p1, p2));

		if (index == 0)
		{
			minOffset = minv(p0, minv(p1, p2));
			maxOffset = maxv(p0, maxv(p1, p2));
			distance = etMax(p0.length(), etMax(p1.length(), p2.length()));
			++index;
		}
		else
		{
			minOffset = minv(minOffset, p0);
			minOffset = minv(minOffset, p1);
			minOffset = minv(minOffset, p2);
			maxOffset = maxv(maxOffset, p0);
			maxOffset = maxv(maxOffset, p1);
			maxOffset = maxv(maxOffset, p2);
			distance = etMax(p0.length(), etMax(p1.length(), p2.length()));
		}
	}
	
	_size = maxv(maxOffset - minOffset, vec3(std::numeric_limits<float>::epsilon()));
	_center = 0.5f * (maxOffset + minOffset);
	_radius = distance;
}

SupportMesh* SupportMesh::duplicate()
{
	SupportMesh* result = sharedObjectFactory().createObject<SupportMesh>(name(), vertexArrayObject(),
		material(), startIndex(), numIndexes(), parent());

	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	result->_size = _size;
	result->_center = _center;
	result->_radius = _radius;
	result->_data = _data;
	
	return result;
}

float SupportMesh::finalTransformScale()
{
	return 1.0f / std::pow(std::abs(finalInverseTransform().mat3().determinant()), 1.0f / 3.0f);
}

Sphere SupportMesh::sphere() 
{
	return Sphere(finalTransform() * _center, finalTransformScale() * _radius);
}

const AABB& SupportMesh::aabb()
{
	if (_shouldBuildAABB)
	{
		const mat4& ft = finalTransform();
		_cachedAABB = AABB(ft * _center, absv(ft.rotationMultiply(_size)));
		_shouldBuildAABB = false;
	}
	
	return _cachedAABB;
}

OBB SupportMesh::obb()
{
	mat4 ft = finalTransform();
	mat3 r = ft.mat3();
	vec3 s = removeMatrixScale(r);
	return OBB(ft * _center, s * _size, r);
}

void SupportMesh::serialize(std::ostream& stream, SceneVersion version)
{
	serializeFloat(stream, _radius);
	serializeVector(stream, _size);
	serializeVector(stream, _center);
	serializeUInt64(stream, _data.size());
	stream.write(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
	Mesh::serialize(stream, version);
}

void SupportMesh::deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version)
{
	_radius = deserializeFloat(stream);
	_size = deserializeVector<vec3>(stream);
	_center = deserializeVector<vec3>(stream);
	
	uint64_t dataSize = (version < SceneVersion_1_1_0) ? deserializeUInt32(stream) : deserializeUInt64(stream);
	_data.resize(dataSize);

	if (version <= SceneVersion_1_0_1)
	{
		for (size_t i = 0, e = _data.size(); i != e; ++i)
		{
			vec3 vertices[3];
			stream.read(reinterpret_cast<char*>(vertices), sizeof(vertices));
			_data[i] = triangle(vertices[0], vertices[1], vertices[2]);
		}
	}
	else 
	{
		stream.read(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
	}

	Mesh::deserialize(stream, factory, version);
}

void SupportMesh::transformInvalidated()
{
	_shouldBuildAABB = true;
}
