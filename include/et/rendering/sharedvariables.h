/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et/rendering/interface/databuffer.h>

namespace et
{
	class RenderInterface;
	class SharedVariables
	{
	public:
		void init(RenderInterface*);
		void shutdown();

		DataBuffer::Pointer buffer();
		void flushBuffer();
		
		void loadCameraProperties(const Camera::Pointer&);
		void loadLightProperties(const Camera::Pointer&);

	private:
		DataBuffer::Pointer _buffer;
		BinaryDataStorage _localData;
		bool _bufferDataValid = false;
	};
}
