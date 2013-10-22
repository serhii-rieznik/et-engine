/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
	namespace audio
	{
		class Description : public LoadableObject
		{
		public:
			ET_DECLARE_POINTER(Description)
			
		public:
			Description() :	
				duration(0.0f), format(0), channels(0), bitDepth(0), sampleRate(0) { }

		public:
			float duration;

			size_t format;
			size_t channels;
			size_t bitDepth;
			size_t sampleRate;

			BinaryDataStorage data;
		};
	};
}