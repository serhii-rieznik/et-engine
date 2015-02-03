/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/scene3d/mesh.h>
#include <et/scene3d/storage.h>

using namespace et;
using namespace et::s3d;

const std::string Mesh::defaultMeshName = "mesh";

static IndexBuffer _emptyIndexBuffer;
static VertexBuffer::Pointer _emptyVertexBuffer;

Mesh::Mesh(const std::string& name, Element* parent) :
	RenderableElement(name, parent) { }

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const Material::Pointer& mat,
	uint32_t startIndex, uint32_t numIndexes, Element* parent) : RenderableElement(aName, parent), _vao(vao),
	_startIndex(startIndex), _numIndexes(numIndexes)
{
	setMaterial(mat);
}

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const Material::Pointer& mat,
	uint32_t start, uint32_t num, const VertexStorage::Pointer& storage, const IndexArray::Pointer& ia,
	Element* parent) : RenderableElement(aName, parent), _vao(vao), _startIndex(start), _numIndexes(num),
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
	
	if (_vertexStorage.invalid())
	{
		log::warning("Unable to calculate support data for mesh, storage is invalid");
		return;
	}

	if (_indexArray.invalid())
	{
		log::warning("Unable to calculate support data for mesh, index array is invalid");
		return;
	}
	
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
	
	_supportData.dimensions = maxVertex - minVertex;
	_supportData.minMaxCenter = 0.5f * (minVertex + maxVertex);
	_supportData.averageCenter /= static_cast<float>(_numIndexes);
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

void Mesh::serialize(std::ostream& stream, SceneVersion version)
{
	std::string vbId = (_vao.valid() && _vao->vertexBuffer().valid()) ?
		intToStr(_vao->vertexBuffer()->sourceTag() & 0xffffffff) : "0";
	
	std::string ibId = (_vao.valid() && _vao->indexBuffer().valid()) ?
		intToStr(_vao->indexBuffer()->sourceTag() & 0xffffffff) : "0";
	
	std::string ibName = "ib-" + ibId;
	std::string vbName = "vb-" + vbId;
	std::string vaoName = "vao-" + vbId + "-" + ibId;

	serializeString(stream, vaoName);
	serializeString(stream, vbName);
	serializeString(stream, ibName);
	
	serializeUInt64(stream, reinterpret_cast<uintptr_t>(material().ptr()));
	
	serializeUInt32(stream, _startIndex);
	serializeUInt32(stream, _numIndexes);

	serializeUInt64(stream, _lods.size());
	for (auto i : _lods)
	{
		serializeUInt32(stream, i.first);
		i.second->serialize(stream, version);
	}

	serializeGeneralParameters(stream, version);
	serializeChildren(stream, version);
}

void Mesh::deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version)
{
	_vaoName = deserializeString(stream);
	_vbName = deserializeString(stream);
	_ibName = deserializeString(stream);
	
	uint64_t materialId = (version < SceneVersion_1_1_0) ? deserializeUInt32(stream) : deserializeUInt64(stream);
	setMaterial(factory->materialWithId(materialId));
	
	setVertexArrayObject(factory->vaoWithIdentifiers(_vbName, _ibName));
	
	setIndexArray(factory->primaryIndexArray());
	setVertexStorage(factory->vertexStorageForVertexBuffer(_vbName));

	_startIndex = deserializeUInt32(stream);
	_numIndexes = deserializeUInt32(stream);

	uint64_t numLods = (version < SceneVersion_1_1_0) ? deserializeUInt32(stream) : deserializeUInt64(stream);
	for (uint64_t i = 0; i < numLods; ++i)
	{
		uint32_t level = deserializeUInt32(stream);
		Mesh::Pointer p = factory->createElementOfType(ElementType_Mesh, 0);
		p->deserialize(stream, factory, version);
		attachLod(level, p);
	}

	deserializeGeneralParameters(stream, version);
	deserializeChildren(stream, factory, version);
}

void Mesh::attachLod(uint32_t level, Mesh::Pointer mesh)
{
	_lods[level] = mesh;
	mesh->setActive(false);
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
