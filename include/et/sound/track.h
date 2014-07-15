/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
            Track(const std::string& fileName);

			ET_DENY_COPY(Track)

		private:
			unsigned int buffer() const;
			unsigned int* buffers() const;
            
		private:
			friend class Player;
			friend class Manager;
			
		private:
           TrackPrivate* _private;
        };
	}
}
