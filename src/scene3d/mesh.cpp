/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/core/conversion.h>
#include <et/scene3d/mesh.h>
#include <et/scene3d/storage.h>

using namespace et;
using namespace et::s3d;

const std::string Mesh::defaultMeshName = "mesh";

static IndexBuffer _emptyIndexBuffer;
static VertexBuffer::Pointer _emptyVertexBuffer;

Mesh::Mesh(const std::string& name, BaseElement* parent) :
	RenderableElement(name, parent) { }

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const Material::Pointer& mat,
	uint32_t startIndex, uint32_t numIndexes, BaseElement* parent) : RenderableElement(aName, parent), 
	_vao(vao), _startIndex(startIndex), _numIndexes(numIndexes)
{
	setMaterial(mat);
}

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const Material::Pointer& mat,
	uint32_t start, uint32_t num, const VertexStorage::Pointer& storage, const IndexArray::Pointer& ia,
	BaseElement* parent) : RenderableElement(aName, parent), _vao(vao), _startIndex(start), _numIndexes(num),
	_vertexStorage(storage), _indexArray(ia)
{
	setMaterial(mat);
	calculateSupportData();
}

void Mesh::calculateSupportData()
{
	_supportData.dimensions = vec3(0.0f);
	_supportData.minMaxCenter = vec3(0.0f);
	_supportData.averageCenter = vec3(0.0f);
	
	if ((_numIndexes == 0) || _vertexStorage.invalid() || _indexArray.invalid()) return;
	
	if (!_vertexStorage->hasAttributeWithType(VertexAttributeUsage::Position, VertexAttributeType::Vec3))
	{
		log::warning("Unable to calculate support data for mesh, storage not containing position of type vec3");
		return;
	}
	
	vec3 minVertex( std::numeric_limits<float>::max());
	vec3 maxVertex(-std::numeric_limits<float>::max());
	const auto pos = _vertexStorage->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, 0);
	for (uint32_t i = 0; i < _numIndexes; ++i)
	{
		size_t index = _indexArray->getIndex(_startIndex + i);
		const auto& v = pos[index];
		minVertex = minv(minVertex, v);
		maxVertex = maxv(maxVertex, v);
		_supportData.averageCenter += v;
	}
	
	vec3 maxExtent = maxv(absv(maxVertex), absv(minVertex));
	
	_supportData.dimensions = maxVertex - minVertex;
	_supportData.minMaxCenter = 0.5f * (minVertex + maxVertex);
	_supportData.averageCenter /= static_cast<float>(_numIndexes);
	_supportData.boundingSphereRadius = etMax(etMax(maxExtent.x, maxExtent.y), maxExtent.z);
	_supportData.valid = true;

	ET_ASSERT(!isnan(_supportData.averageCenter.x));
}

Mesh* Mesh::duplicate()
{
	Mesh* result = sharedObjectFactory().createObject<Mesh>(name(), _vao, material(),
		_startIndex, _numIndexes, parent());
	
	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	return result;
}

void Mesh::setVertexBuffer(VertexBuffer::Pointer vb)
{
	if (_vao.valid())
		_vao->setVertexBuffer(vb);
}

void Mesh::setIndexBuffer(IndexBuffer ib)
{
	if (_vao.valid())
		_vao->setIndexBuffer(ib);
}

void Mesh::setVertexArrayObject(VertexArrayObject vao)
{
	ET_ASSERT(vao.valid());
	_vao = vao;
}

void Mesh::setVertexStorage(VertexStorage::Pointer vs)
{
	_vertexStorage = vs;
}

void Mesh::setIndexArray(IndexArray::Pointer ia)
{
	_indexArray = ia;
}

void Mesh::serialize(Dictionary stream, const std::string& basePath)
{
	stream.setIntegerForKey(kStartIndex, _startIndex);
	stream.setIntegerForKey(kIndexesCount, _numIndexes);

	if (_vertexStorage.valid())
		stream.setStringForKey(kVertexStorageName, _vertexStorage->name());

	if (_indexArray.valid())
		stream.setStringForKey(kIndexArrayName, _indexArray->name());

	if (!_lods.empty())
	{
		Dictionary lodsDictionary;
		for (auto& kv : _lods)
		{
			Dictionary lodDictionary;
			kv.second->serialize(lodDictionary, basePath);
			lodsDictionary.setDictionaryForKey(intToStr(kv.first), lodDictionary);
		}
		stream.setDictionaryForKey(kLods, lodsDictionary);
	}

	if (_supportData.valid)
	{
		Dictionary supportDataDictionary;
		supportDataDictionary.setArrayForKey(kMinMaxCenter, vec3ToArray(_supportData.minMaxCenter));
		supportDataDictionary.setArrayForKey(kAverageCenter, vec3ToArray(_supportData.averageCenter));
		supportDataDictionary.setArrayForKey(kDimensions, vec3ToArray(_supportData.dimensions));
		supportDataDictionary.setFloatForKey(kBoundingSphereRadius, _supportData.boundingSphereRadius);
		stream.setDictionaryForKey(kSupportData, supportDataDictionary);
	}

	RenderableElement::serialize(stream, basePath);
}

void Mesh::deserialize(Dictionary stream, ElementFactory* factory)
{
	RenderableElement::deserialize(stream, factory);
}

void Mesh::attachLod(uint32_t level, Mesh::Pointer mesh)
{
	_lods[level] = mesh;
}

void Mesh::cleanupLodChildren()
{
	for (auto i : _lods)
	{
		if (i.second->parent() == this)
			i.second->setParent(nullptr);
	}
}

VertexArrayObject& Mesh::vertexArrayObject()
{
	return currentLod()->_vao; 
}

const VertexArrayObject& Mesh::vertexArrayObject() const
{
	return currentLod()->_vao; 
}

VertexBuffer::Pointer& Mesh::vertexBuffer() 
{
	VertexArrayObject& vao = vertexArrayObject();
	return vao.valid() ? vao->vertexBuffer() : _emptyVertexBuffer; 
}

const VertexBuffer::Pointer& Mesh::vertexBuffer() const
{
	const VertexArrayObject& vao = vertexArrayObject();
	return vao.valid() ? vao->vertexBuffer() : _emptyVertexBuffer; 
}

IndexBuffer& Mesh::indexBuffer() 
{
	VertexArrayObject& vao = vertexArrayObject();
	return vao.valid() ? vao->indexBuffer() : _emptyIndexBuffer; 
}

const IndexBuffer& Mesh::indexBuffer() const
{
	VertexArrayObject vao = vertexArrayObject();
	return vao.valid() ? vao->indexBuffer() : _emptyIndexBuffer; 
}

uint32_t Mesh::startIndex() const 
{
	return currentLod()->_startIndex; 
}

uint32_t Mesh::numIndexes() const
{
	return currentLod()->_numIndexes; 
}

void Mesh::setStartIndex(uint32_t index)
{
	currentLod()->_startIndex = index; 
}

void Mesh::setNumIndexes(uint32_t num)
{
	currentLod()->_numIndexes = num; 
}

const Mesh* Mesh::currentLod() const
{
	if (_selectedLod == 0) return this;
	
	auto i = _lods.find(_selectedLod);
	return (i == _lods.end()) ? this : i->second.ptr();
}

Mesh* Mesh::currentLod()
{
	if (_selectedLod == 0) return this;

	auto i = _lods.find(_selectedLod);
	return (i == _lods.end()) ? this : i->second.ptr();
}

void Mesh::setLod(uint32_t level)
{
	auto i = _lods.find(level);
	_selectedLod = (i == _lods.end()) ? 0 : level;
}

void Mesh::transformInvalidated()
{
	_shouldUpdateBoundingBox = true;
	_shouldUpdateBoundingSphere = true;
}

float Mesh::finalTransformScale()
{
	return 1.0f / std::pow(std::abs(finalInverseTransform().mat3().determinant()), 1.0f / 3.0f);
}

const Sphere& Mesh::boundingSphere()
{
	if (_shouldUpdateBoundingSphere && _supportData.valid)
	{
		_shouldUpdateBoundingSphere = false;
		
		_cachedBoundingSphere = Sphere(finalTransform() * _supportData.averageCenter,
			finalTransformScale() * _supportData.boundingSphereRadius);
	}
	
	return _cachedBoundingSphere;
}

const AABB& Mesh::boundingBox()
{
	if (_shouldUpdateBoundingBox && _supportData.valid)
	{
		AABB originalAABB = AABB(_supportData.averageCenter, 0.5f * _supportData.dimensions);
		
		vec3 minVertex(+std::numeric_limits<float>::max());
		vec3 maxVertex(-std::numeric_limits<float>::max());
		
		const auto& ft = finalTransform();
		for (size_t i = 0; i < AABBCorner_max; ++i)
		{
			vec3 transformedCorner = ft * originalAABB.corners[i];
			minVertex = minv(minVertex, transformedCorner);
			maxVertex = maxv(maxVertex, transformedCorner);
		}
		
		_cachedBoundingBox = AABB(0.5f * (maxVertex + minVertex), 0.5f * (maxVertex - minVertex));
		_shouldUpdateBoundingBox = false;
	}
	
	return _cachedBoundingBox;
}
