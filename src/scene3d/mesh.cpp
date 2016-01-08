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

Mesh::Mesh(const std::string& aName, const SceneMaterial::Pointer& mat, BaseElement* parent) :
	RenderableElement(aName, mat, parent)
{
	_undeformedTransformationMatrices.resize(4);
}

void Mesh::calculateSupportData()
{
	_boundingBox = BoundingBox();
	
	float processedBatches = 0.0f;
	vec3 minVertex( std::numeric_limits<float>::max());
	vec3 maxVertex(-std::numeric_limits<float>::max());
	
	for (auto& rb : renderBatches())
	{
		const auto& vs = rb->vertexStorage();
		const auto& ia = rb->indexArray();
		if (vs.valid() && ia.valid() && vs->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3))
		{
			rb->calculateBoundingBox();
			minVertex = minv(minVertex, rb->minExtent());
			maxVertex = maxv(maxVertex, rb->minExtent());
			processedBatches += 1.0f;
		}
	}
	
	if (processedBatches > 0.0f)
	{
		vec3 dimensions = maxVertex - minVertex;
		_boundingBox = BoundingBox(0.5f * (minVertex + maxVertex), 0.5f * dimensions);
		_boundingSphereRadius =  0.5f * etMax(etMax(dimensions.x, dimensions.y), dimensions.z);
	}
}

Mesh* Mesh::duplicate()
{
	ET_FAIL("Not implemented");
	return nullptr;
}

void Mesh::duplicateMeshPropertiesToMesh(s3d::Mesh* result)
{
	result->_supportData = _supportData;
	result->_deformer = _deformer; // TODO: clone deformer
}

void Mesh::serialize(Dictionary stream, const std::string& basePath)
{
	// TODO : write deformer
	RenderableElement::serialize(stream, basePath);
}

void Mesh::deserialize(Dictionary stream, SerializationHelper* helper)
{
	RenderableElement::deserialize(stream, helper);
	/*
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
		_boundingSphereRadius = supportData.floatForKey(kBoundingSphereRadius)->content;
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
	*/
}

void Mesh::transformInvalidated()
{
	_supportData.shouldUpdateBoundingBox = true;
	_supportData.shouldUpdateBoundingSphere = true;
	_supportData.shouldUpdateBoundingSphereUntransformed = true;
}

float Mesh::finalTransformScale()
{
	return 1.0f / std::pow(std::abs(finalInverseTransform().mat3().determinant()), 1.0f / 3.0f);
}

const Sphere& Mesh::boundingSphereUntransformed()
{
	if (_supportData.shouldUpdateBoundingSphereUntransformed)
	{
		_supportData.untranfromedBoundingSphere = Sphere(_boundingBox.center,
			_boundingSphereRadius);
		_supportData.shouldUpdateBoundingSphereUntransformed = false;
	}
	return _supportData.untranfromedBoundingSphere;
}

const Sphere& Mesh::boundingSphere()
{
	if (_supportData.shouldUpdateBoundingSphere)
	{
		const auto& ft = finalTransform();
		_supportData.tranfromedBoundingSphere = Sphere(ft * _boundingBox.center,
			finalTransformScale() * _boundingSphereRadius);
		_supportData.shouldUpdateBoundingSphere = false;
	}
	return _supportData.tranfromedBoundingSphere;
}

const BoundingBox& Mesh::tranformedBoundingBox()
{
	if (_supportData.shouldUpdateBoundingBox)
	{
		BoundingBox::Corners corners;
		_boundingBox.calculateTransformedCorners(corners, finalTransform().mat3());
		
		vec3 minVertex(+std::numeric_limits<float>::max());
		vec3 maxVertex(-std::numeric_limits<float>::max());
		for (uint32_t i = 0; i < 8; ++i)
		{
			minVertex = minv(minVertex, corners[i]);
			maxVertex = maxv(maxVertex, corners[i]);
		}
		_supportData.transformedBoundingBox = BoundingBox(0.5f * (maxVertex + minVertex), 0.5f * maxv(vec3(0.0002f), maxVertex - minVertex));
		_supportData.shouldUpdateBoundingBox = false;
	}
	
	return _supportData.transformedBoundingBox;
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

template <DataType attribType>
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
		case DataType::Float:
			copyAttributeWithType<DataType::Float>(from, to, attrib);
			break;
		case DataType::Int:
			copyAttributeWithType<DataType::Int>(from, to, attrib);
			break;
		case DataType::Vec2:
			copyAttributeWithType<DataType::Vec2>(from, to, attrib);
			break;
		case DataType::Vec3:
			copyAttributeWithType<DataType::Vec3>(from, to, attrib);
			break;
		case DataType::Vec4:
			copyAttributeWithType<DataType::Vec4>(from, to, attrib);
			break;
		case DataType::IntVec2:
			copyAttributeWithType<DataType::IntVec2>(from, to, attrib);
			break;
		case DataType::IntVec3:
			copyAttributeWithType<DataType::IntVec3>(from, to, attrib);
			break;
		case DataType::IntVec4:
			copyAttributeWithType<DataType::IntVec4>(from, to, attrib);
			break;
		default:
			ET_FAIL("Unhandled attribute type");
	}
}

void copyVector3Rotated(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const mat4& transform)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<DataType::Vec3>(attrib, 0);
	auto c1 = to->accessData<DataType::Vec3>(attrib, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
		c1[i] = transform.rotationMultiply(c0[i]);
}

void copyVector3Transformed(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const mat4& transform)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<DataType::Vec3>(attrib, 0);
	auto c1 = to->accessData<DataType::Vec3>(attrib, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
		c1[i] = transform * c0[i];
}

void skinVector3Rotated(VertexStorage::Pointer from, VertexStorage::Pointer to, VertexAttributeUsage attrib,
	const std::vector<mat4>& transforms)
{
	if (!from->hasAttribute(attrib)) return;
	
	auto c0 = from->accessData<DataType::Vec3>(attrib, 0);
	auto c1 = to->accessData<DataType::Vec3>(attrib, 0);
	auto bi = to->accessData<DataType::IntVec4>(VertexAttributeUsage::BlendIndices, 0);
	auto bw = to->accessData<DataType::Vec4>(VertexAttributeUsage::BlendWeights, 0);
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
	
	auto c0 = from->accessData<DataType::Vec3>(attrib, 0);
	auto c1 = to->accessData<DataType::Vec3>(attrib, 0);
	auto bi = to->accessData<DataType::IntVec4>(VertexAttributeUsage::BlendIndices, 0);
	auto bw = to->accessData<DataType::Vec4>(VertexAttributeUsage::BlendWeights, 0);
	for (uint32_t i = 0; i < from->capacity(); ++i)
	{
		c1[i] = (transforms[bi[i][0]] * c0[i]) * bw[i][0] + (transforms[bi[i][1]] * c0[i]) * bw[i][1] +
			(transforms[bi[i][2]] * c0[i]) * bw[i][2] +  (transforms[bi[i][3]] * c0[i]) * bw[i][3];
	}
}

bool Mesh::skinned() const
{
	ET_FAIL("TODO");
	/*
	 * TODO
	 *
	return _vertexStorage->hasAttribute(VertexAttributeUsage::BlendIndices) &&
		_vertexStorage->hasAttribute(VertexAttributeUsage::BlendWeights);
	// */
	return false;
}

VertexStorage::Pointer Mesh::bakeDeformations()
{
	ET_FAIL("TODO");
	
	/*
	 * TODO
	 *
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
	// */
	return VertexStorage::Pointer();
}
