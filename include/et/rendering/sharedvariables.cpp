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
 float4 cameraPosition;
 float4 cameraDirection;
 float4 cameraUp;
 float4 lightPosition;
 */

enum : uint32_t
{
	SharedVariablesDataSize = AlignUpTo<3 * sizeof(mat4) + 4 * sizeof(vec4), 16>::value
};

void SharedVariables::init(RenderInterface* renderer)
{
	_localData.resize(SharedVariablesDataSize);
	_buffer = renderer->createDataBuffer("SharedVariablesBuffer", SharedVariablesDataSize);
}

void SharedVariables::shutdown()
{
	_buffer.reset(nullptr);
	_localData.resize(0);
}

void SharedVariables::loadCameraProperties(const Camera::Pointer& cam)
{
	ET_ASSERT(cam.valid());

	mat4* mPtr = reinterpret_cast<mat4*>(_localData.binary());
	*mPtr++ = cam->viewProjectionMatrix();
	*mPtr++ = cam->projectionMatrix();
	*mPtr++ = cam->viewMatrix();

	vec4* vPtr = reinterpret_cast<vec4*>(mPtr);
	*vPtr++ = vec4(cam->position(), 1.0f);
	*vPtr++ = vec4(cam->direction(), 0.0f);
	*vPtr++ = vec4(cam->up(), 0.0f);

	_bufferDataValid = false;
}

void SharedVariables::loadLightProperties(const Camera::Pointer& light)
{
	ET_ASSERT(light.valid());

	vec4* vPtr = reinterpret_cast<vec4*>(_localData.binary());
	*(vPtr + 15) = vec4(light->position(), 0.0f);
	_bufferDataValid = false;
}

DataBuffer::Pointer SharedVariables::buffer()
{
	return _buffer;
}

void SharedVariables::flushBuffer()
{
	if (!_bufferDataValid)
	{
		_buffer->setData(_localData.binary(), 0, _localData.size());
	}
	_bufferDataValid = true;
}

}
