/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/renderer.h>
#include <et/rendering/sharedvariables.h>

namespace et
{
/*
 * Layout
 *
 float4x4 viewProjection;
 float4x4 projection;
 float4x4 view;
 packed_float3 cameraPosition;
 packed_float3 cameraDirection;
 packed_float3 cameraUp;
 */

enum : uint32_t
{
	SharedVariablesDataSize = alignUpTo(3 * sizeof(mat4) + 3 * sizeof(vec3), 16)
};

void SharedVariables::init(RenderInterface* renderer)
{
	et::sharedBlockAllocator().allocate(10);
	
	_localData.resize(SharedVariablesDataSize);
	_buffer = renderer->createDataBuffer("SharedVariablesBuffer", SharedVariablesDataSize);
}

void SharedVariables::shutdown()
{
	_buffer.reset(nullptr);
	_localData.resize(0);
}

void SharedVariables::loadCameraProperties(const Camera& cam)
{
	mat4* mPtr = reinterpret_cast<mat4*>(_localData.binary());
	*mPtr++ = cam.viewProjectionMatrix();
	*mPtr++ = cam.projectionMatrix();
	*mPtr++ = cam.viewMatrix();

	vec3* vPtr = reinterpret_cast<vec3*>(mPtr);
	*vPtr++ = cam.position();
	*vPtr++ = cam.direction();
	*vPtr++ = cam.up();

	_bufferDataValid = false;
}

DataBuffer::Pointer SharedVariables::buffer()
{
	if (!_bufferDataValid)
	{
		_buffer->setData(_localData.binary(), 0, _localData.size());
		_bufferDataValid = true;
	}

	_bufferDataValid = true;
	return _buffer;
}

}
