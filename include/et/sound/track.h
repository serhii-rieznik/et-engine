/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

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
			Track(const std::string& fileName);
			~Track();

			float duration() const;
			
			size_t channels() const;
			size_t sampleRate() const;
			size_t bitDepth() const;
			size_t samples() const;
			
			int totalBuffersCount() const;
			int actualBuffersCount() const;
			
			bool streamed() const;
			unsigned int loadNextBuffer();
			
			void preloadBuffers();
			void rewind();
			
		private:
			unsigned int buffer() const;
			unsigned int* buffers() const;
			
		private:
			friend class Player;
			friend class Manager;
			
			ET_DENY_COPY(Track)
			ET_DECLARE_PIMPL(Track, 1536)
        };
	}
}
