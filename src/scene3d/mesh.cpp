/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
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

static IndexBuffer::Pointer _emptyIndexBuffer;
static VertexBuffer::Pointer _emptyVertexBuffer;

Mesh::Mesh(const std::string& name, BaseElement* parent) :
	RenderableElement(name, parent)
{
	_undeformedTransformationMatrices.resize(4);
}

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const SceneMaterial::Pointer& mat,
	uint32_t startIndex, uint32_t numIndexes, BaseElement* parent) : RenderableElement(aName, parent), 
	_vao(vao), _startIndex(startIndex), _numIndexes(numIndexes)
{
	setMaterial(mat);
	_undeformedTransformationMatrices.resize(4);
}

Mesh::Mesh(const std::string& aName, const VertexArrayObject& vao, const SceneMaterial::Pointer& mat,
	uint32_t start, uint32_t num, const VertexStorage::Pointer& storage, const IndexArray::Pointer& ia,
	BaseElement* parent) : RenderableElement(aName, parent), _vao(vao), _startIndex(start), _numIndexes(num),
	_vertexStorage(storage), _indexArray(ia)
{
	setMaterial(mat);
	calculateSupportData();
	_undeformedTransformationMatrices.resize(4);
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
		uint32_t index = _indexArray->getIndex(_startIndex + i);
		const auto& v = pos[index];
		minVertex = minv(minVertex, v);
		maxVertex = maxv(maxVertex, v);
		_supportData.averageCenter += v;
	}
	
	_supportData.dimensions = maxVertex - minVertex;
	_supportData.minMaxCenter = 0.5f * (minVertex + maxVertex);
	_supportData.averageCenter /= static_cast<float>(_numIndexes);

	_supportData.boundingSphereRadius = 
		0.5f * etMax(etMax(_supportData.dimensions.x, _supportData.dimensions.y), _supportData.dimensions.z);

	_supportData.valid = true;

	ET_ASSERT(!isnan(_supportData.averageCenter.x));
}

Mesh* Mesh::duplicate()
{
	Mesh* result = etCreateObject<Mesh>(name(), _vao, material(),
		_startIndex, _numIndexes, parent());
	
	duplicateMeshPropertiesToMesh(result);
	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	return result;
}

void Mesh::duplicateMeshPropertiesToMesh(s3d::Mesh* result)
{
	result->_supportData = _supportData;
	result->_deformer = _deformer; // TODO: clone deformer
}

void Mesh::setVertexBuffer(VertexBuffer::Pointer vb)
{
	if (_vao.valid())
		_vao->setVertexBuffer(vb);
}

void Mesh::setIndexBuffer(IndexBuffer::Pointer ib)
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

void Mesh::deserialize(Dictionary stream, SerializationHelper* helper)
{
	_startIndex = static_cast<uint32_t>(stream.integerForKey(kStartIndex)->content);
	_numIndexes = static_cast<uint32_t>(stream.integerForKey(kIndexesCount)->content);
	
	if (stream.hasKey(kVertexStorageName))
	{
		auto vertexStorageName = stream.stringForKey(kVertexStorageName)->content;
		_vertexStorage = helper->vertexStorageWithName(vertexStorageName);
		_vao = helper->vertexArrayWithStorageName(vertexStorageName);
	}
	
	if (stream.hasKey(kIndexArrayName))
	{
		_indexArray = helper->indexArrayWithName(stream.stringForKey(kIndexArrayName)->content);
	}
	
	if (stream.hasKey(kSupportData))
	{
		auto supportData = stream.dictionaryForKey(kSupportData);
		_supportData.minMaxCenter = arrayToVec3(supportData.arrayForKey(kMinMaxCenter));
		_supportData.averageCenter = arrayToVec3(supportData.arrayForKey(kAverageCenter));
		_supportData.dimensions = arrayToVec3(supportData.arrayForKey(kDimensions));
		_supportData.boundingSphereRadius = supportData.floatForKey(kBoundingSphereRadius)->content;
		_supportData.valid = true;
	}
	else
	{
		calculateSupportData();
	}
	
	if (stream.hasKey(kLods))
	{
		auto lods = stream.dictionaryForKey(kLods)->content;
		for (const auto& lod : lods)
		{
			auto mesh = s3d::Mesh::Pointer::create(lod.first);
			mesh->deserialize(lod.second, helper);
		}
	}
	
	RenderableElement::deserialize(stream, helper);
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

IndexBuffer::Pointer& Mesh::indexBuffer() 
{
	VertexArrayObject& vao = vertexArrayObject();
	return vao.valid() ? vao->indexBuffer() : _emptyIndexBuffer; 
}

const IndexBuffer::Pointer& Mesh::indexBuffer() const
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
	_shouldUpdateOrientedBoundingBox = true;
	_shouldUpdateBoundingSphereUntransformed = true;
}

float Mesh::finalTransformScale()
{
	return 1.0f / std::pow(std::abs(finalInverseTransform().mat3().determinant()), 1.0f / 3.0f);
}

const Sphere& Mesh::boundingSphereUntransformed()
{
	if (_shouldUpdateBoundingSphereUntransformed && _supportData.valid)
	{
		_cachedBoundingSphereUntransformed = Sphere(_supportData.averageCenter,
			_supportData.boundingSphereRadius);
		_shouldUpdateBoundingSphereUntransformed = false;
	}
	return _cachedBoundingSphereUntransformed;
}

const Sphere& Mesh::boundingSphere()
{
	if (_shouldUpdateBoundingSphere && _supportData.valid)
	{
		const auto& ft = finalTransform();
		_cachedBoundingSphere = Sphere(ft * _supportData.averageCenter,
			finalTransformScale() * _supportData.boundingSphereRadius);
		_shouldUpdateBoundingSphere = false;
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
		for (uint32_t i = 0; i < AABBCorner_max; ++i)
		{
			vec3 transformedCorner = ft * originalAABB.corners[i];
			minVertex = minv(minVertex, transformedCorner);
			maxVertex = maxv(maxVertex, transformedCorner);
		}
		
		_cachedBoundingBox = AABB(0.5f * (maxVertex + minVertex), 0.5f * maxv(vec3(0.0002f), maxVertex - minVertex));
		_shouldUpdateBoundingBox = false;
	}
	
	return _cachedBoundingBox;
}

const OBB& Mesh::orientedBoundingBox()
{
	if (_shouldUpdateOrientedBoundingBox)
	{
		mat4 ft = finalTransform();
		mat3 r = ft.mat3();
		vec3 s = removeMatrixScale(r);
		_cachedOrientedBoundingBox = OBB(ft * _supportData.averageCenter, 0.5f * s * _supportData.dimensions, r);
		_shouldUpdateOrientedBoundingBox = false;
	}

	return _cachedOrientedBoundingBox;
}

const std::vector<mat4>& Mesh::deformationMatrices()
{
	if (_deformer.valid())
		return _deformer->calculateTransformsForMesh(this);
	
	for (uint32_t i = 0; i < 4; ++i)
		_undeformedTransformationMatrices[i] = finalTransform();
	
	return _undeformedTransformationMatrices;
}

/*
 * Bake deformations + stuff for it
 */

template <VertexAttributeType attribType>
void copyAttributeWithType(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib)
{
	auto c0 = from->accessData<attribType>(attrib, 0);
	auto c1 = to->accessData<attribType>(attrib, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
		c1[i] = c0[i];
}

void copyAttribute(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib)
{
	if (!from->hasAttribute(attrib)) return;
	
	switch (from->attributeType(attrib))
	{
		case VertexAttributeType::Float:
			copyAttributeWithType<VertexAttributeType::Float>(from, to, attrib);
			break;
		case VertexAttributeType::Int:
			copyAttributeWithType<VertexAttributeType::Int>(from, to, attrib);
			break;
		case VertexAttributeType::Vec2:
			copyAttributeWithType<VertexAttributeType::Vec2>(from, to, attrib);
			break;
		case VertexAttributeType::Vec3:
			copyAttributeWithType<VertexAttributeType::Vec3>(from, to, attrib);
			break;
		case VertexAttributeType::Vec4:
			copyAttributeWithType<VertexAttributeType::Vec4>(from, to, attrib);
			break;
		case VertexAttributeType::IntVec2:
			copyAttributeWithType<VertexAttributeType::IntVec2>(from, to, attrib);
			break;
		case VertexAttributeType::IntVec3:
			copyAttributeWithType<VertexAttributeType::IntVec3>(from, to, attrib);
			break;
		case VertexAttributeType::IntVec4:
			copyAttributeWithType<VertexAttributeType::IntVec4>(from, to, attrib);
			break;
		default:
			ET_FAIL("Unhandled attribute type");
	}
}

void copyVector3Rotated(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const mat4& transform)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto c1 = to->accessData<VertexAttributeType::Vec3>(attrib, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
		c1[i] = transform.rotationMultiply(c0[i]);
}

void copyVector3Transformed(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const mat4& transform)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto c1 = to->accessData<VertexAttributeType::Vec3>(attrib, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
		c1[i] = transform * c0[i];
}

void skinVector3Rotated(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const std::vector<mat4>& transforms)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto c1 = to->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto bi = to->accessData<VertexAttributeType::IntVec4>(VertexAttributeUsage::BlendIndices, 0);
	auto bw = to->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::BlendWeights, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
	{
		c1[i] =
			transforms[bi[i][0]].rotationMultiply(c0[i]) * bw[i][0] +
			transforms[bi[i][1]].rotationMultiply(c0[i]) * bw[i][1] +
			transforms[bi[i][2]].rotationMultiply(c0[i]) * bw[i][2] +
			transforms[bi[i][3]].rotationMultiply(c0[i]) * bw[i][3];
	}
}

void skinVector3Transformed(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const std::vector<mat4>& transforms)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto c1 = to->accessData<VertexAttributeType::Vec3>(attrib, 0);
	auto bi = to->accessData<VertexAttributeType::IntVec4>(VertexAttributeUsage::BlendIndices, 0);
	auto bw = to->accessData<VertexAttributeType::Vec4>(VertexAttributeUsage::BlendWeights, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
	{
		c1[i] = (transforms[bi[i][0]] * c0[i]) * bw[i][0] + (transforms[bi[i][1]] * c0[i]) * bw[i][1] +
			(transforms[bi[i][2]] * c0[i]) * bw[i][2] +  (transforms[bi[i][3]] * c0[i]) * bw[i][3];
	}
}

bool Mesh::skinned() const
{
	return _vertexStorage->hasAttribute(VertexAttributeUsage::BlendIndices) &&
		_vertexStorage->hasAttribute(VertexAttributeUsage::BlendWeights);
}

VertexStorage::Pointer Mesh::bakeDeformations()
{
	const auto& transforms = deformationMatrices();
	
	VertexStorage::Pointer result =
		VertexStorage::Pointer::create(_vertexStorage->declaration(), _vertexStorage->capacity());
	
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::Color);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::TexCoord0);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::TexCoord1);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::TexCoord2);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::TexCoord3);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::Smoothing);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::BlendIndices);
	copyAttribute(_vertexStorage, result, VertexAttributeUsage::BlendWeights);
	
	if (skinned())
	{
		skinVector3Transformed(_vertexStorage, result, VertexAttributeUsage::Position, transforms);
		skinVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Normal, transforms);
		skinVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Tangent, transforms);
		skinVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Binormal, transforms);
	}
	else
	{
		const mat4& ft = finalTransform();
		copyVector3Transformed(_vertexStorage, result, VertexAttributeUsage::Position, ft);
		copyVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Normal, ft);
		copyVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Tangent, ft);
		copyVector3Rotated(_vertexStorage, result, VertexAttributeUsage::Binormal, ft);
	}
	
	return result;
}
