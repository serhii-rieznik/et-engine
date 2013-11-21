/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/sound/formats.h>

namespace et
{
    namespace audio
    {
        class TrackPrivate;
        class Track : public LoadableObject
        {
        public:
			ET_DECLARE_POINTER(Track)
            
		public:
			~Track();

			float duration() const;
			
			size_t channels() const;
			size_t sampleRate() const;
			size_t bitDepth() const;
			
			bool streamed() const;
			unsigned int loadNextBuffer();
			
			void preloadBuffers();
			void rewind();
			
		private:
            Track(const std::string& fileName);

			ET_DENY_COPY(Track)

		private:
			unsigned int buffer() const;
			unsigned int* buffers() const;
			int buffersCount() const;
            
		private:
			friend class Player;
			friend class Manager;
			
		private:
           TrackPrivate* _private;
        };
	}
}