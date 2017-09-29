/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/drawer/common.h>

namespace et
{
namespace s3d
{

class DebugDrawer : public Object, public FlagsHolder
{
public:
	ET_DECLARE_POINTER(DebugDrawer);

public:
	DebugDrawer(const RenderInterface::Pointer&);

	void begin();
	void drawBoundingBox(const BoundingBox&, const mat4& transfrom, const vec4& color);
	void drawViewProjectionMatrix(const mat4&, const vec4& color);
	void drawCameraFrustum(const Camera::Pointer&, const vec4& color);
	void submitBatches(RenderPass::Pointer);

private:
	void drawBoudingBoxCorners(const BoundingBox::Corners&, const vec4&);

	void beginRenderBatch();
	void appendLine(const vec3& pFrom, const vec3& pTo, const vec4& color);
	void endRenderBatch();

private:
	enum : uint32_t
	{
		Capacity = 8 * 1024,
	};
	
	struct DebugVertex
	{
		vec3 pos;
		vec4 color;
		DebugVertex(const vec3& p, const vec4& c) :
			pos(p), color(c) { }
	};
	Material::Pointer _material;
	RenderInterface::Pointer _renderer;
	Buffer::Pointer _vertexBuffer;
	VertexStream::Pointer _linesStream;
	DebugVertex* _mappedData = nullptr;
	Vector<RenderBatch::Pointer> _batches;
	uint32_t _verticesAdded = 0;
	uint32_t _batchVertices = 0;
	uint32_t _firstBatchVertex = 0;
};

}
}
