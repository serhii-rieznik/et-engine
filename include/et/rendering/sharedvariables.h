/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/interface/databuffer.h>

namespace et
{
	class Camera;
	class RenderInterface;
	class SharedVariables
	{
	public:
		void init(RenderInterface*);
		void shutdown();

		DataBuffer::Pointer buffer();

		void loadCameraProperties(const Camera&);

	private:
		DataBuffer::Pointer _buffer;
		BinaryDataStorage _localData;
		bool _bufferDataValid = false;
	};
}
