/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/debugdrawer.h>
#include <et/rendering/rendercontext.h>
#include <et/app/application.h>

namespace et
{
namespace s3d
{

DebugDrawer::DebugDrawer(const RenderInterface::Pointer& renderer) :
	_renderer(renderer)
{
	Buffer::Description vbDesc;
	vbDesc.size = Capacity * sizeof(DebugVertex);
	vbDesc.location = Buffer::Location::Host;
	vbDesc.usage = Buffer::Usage::Vertex;
	_vertexBuffer = _renderer->createBuffer("debug-vb", vbDesc);
	
	Buffer::Description ibDesc;
	ibDesc.size = Capacity * sizeof(uint32_t);
	ibDesc.location = Buffer::Location::Device;
	ibDesc.usage = Buffer::Usage::Vertex;
	ibDesc.initialData.resize(ibDesc.size);
	uint32_t* indices = reinterpret_cast<uint32_t*>(ibDesc.initialData.data());
	for (uint32_t i = 0; i < Capacity; ++i)
		indices[i] = i;
	Buffer::Pointer ib = _renderer->createBuffer("debug-vb", ibDesc);

	VertexDeclaration decl;
	decl.push_back(VertexAttributeUsage::Position, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);

	_linesStream = VertexStream::Pointer::create();
	_linesStream->setVertexBuffer(_vertexBuffer, decl);
	_linesStream->setIndexBuffer(ib, IndexArrayFormat::Format_32bit, PrimitiveType::Lines);

	_material = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/debug.json"));
}

void DebugDrawer::begin()
{
	_verticesAdded = 0;
	_mappedData = reinterpret_cast<DebugVertex*>(_vertexBuffer->map(0, Capacity * sizeof(DebugVertex)));
}

void DebugDrawer::submitBatches(RenderPass::Pointer pass)
{
	if (_verticesAdded > 0)
		_vertexBuffer->modifyRange(0, sizeof(DebugVertex) * _verticesAdded);

	_vertexBuffer->unmap();
	_mappedData = nullptr;

	pass->setSharedVariable(ObjectVariable::WorldTransform, identityMatrix);
	for (const RenderBatch::Pointer& batch : _batches)
		pass->pushRenderBatch(batch);

	_batches.clear();
}

void DebugDrawer::drawBoundingBox(const BoundingBox& bbox, const mat4& transform, const vec4& color)
{
	BoundingBox::Corners corners;
	bbox.calculateCorners(corners);
	for (vec3& c : corners)
		c = transform * c;

	beginRenderBatch();
	drawBoudingBoxCorners(corners, color);
	endRenderBatch();
}

void DebugDrawer::drawViewProjectionMatrix(const mat4& dm, const vec4& color)
{
	float zNear = Camera::zeroClipRange ? 0.0f : -1.0f;
	float zFar = 1.0f;
	mat4 m = dm.inverted();

	BoundingBox::Corners corners;
	corners[0] = m * vec3(-1.0f, -1.0f, zNear);
	corners[1] = m * vec3(+1.0f, -1.0f, zNear);
	corners[2] = m * vec3(-1.0f, +1.0f, zNear);
	corners[3] = m * vec3(+1.0f, +1.0f, zNear);
	corners[4] = m * vec3(-1.0f, -1.0f, zFar);
	corners[5] = m * vec3(+1.0f, -1.0f, zFar);
	corners[6] = m * vec3(-1.0f, +1.0f, zFar);
	corners[7] = m * vec3(+1.0f, +1.0f, zFar);

	beginRenderBatch();
	drawBoudingBoxCorners(corners, color);
	endRenderBatch();
}

void DebugDrawer::drawCameraFrustum(const Camera::Pointer& cam, const vec4& color)
{
	beginRenderBatch();
	drawBoudingBoxCorners(cam->frustum().corners(), color);
	endRenderBatch();
}

void DebugDrawer::drawBoudingBoxCorners(const BoundingBox::Corners& corners, const vec4& color)
{
	appendLine(corners[0], corners[1], color);
	appendLine(corners[2], corners[3], color);
	appendLine(corners[0], corners[2], color);
	appendLine(corners[1], corners[3], color);
	appendLine(corners[0 + 4], corners[1 + 4], color);
	appendLine(corners[2 + 4], corners[3 + 4], color);
	appendLine(corners[0 + 4], corners[2 + 4], color);
	appendLine(corners[1 + 4], corners[3 + 4], color);
	appendLine(corners[0], corners[0 + 4], color);
	appendLine(corners[1], corners[1 + 4], color);
	appendLine(corners[2], corners[2 + 4], color);
	appendLine(corners[3], corners[3 + 4], color);
}

void DebugDrawer::beginRenderBatch()
{
	_firstBatchVertex = _verticesAdded;
	_batchVertices = 0;
}

void DebugDrawer::appendLine(const vec3& pFrom, const vec3& pTo, const vec4& color)
{
	_mappedData[_verticesAdded++] = DebugVertex(pFrom, color);
	_mappedData[_verticesAdded++] = DebugVertex(pTo, color);
	_batchVertices += 2;
}

void DebugDrawer::endRenderBatch()
{
	if (_batchVertices > 0)
	{
		_batches.emplace_back(_renderer->allocateRenderBatch(_material->instance(), _linesStream, _firstBatchVertex, _batchVertices));
	}
}

}
}
