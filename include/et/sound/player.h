/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/animator.h>
#include <et/sound/track.h>

namespace et
{
    namespace audio
    {
        class PlayerPrivate;
        class Player : public Object, public EventReceiver
        {
        public:
			ET_DECLARE_POINTER(Player)

		public:
			Player();
			Player(Track::Pointer track);
			
			~Player();

			void play(bool looped = false);
            void play(Track::Pointer, bool looped = false);
			void pause();
			void stop();
			void rewind();
			
			void setVolume(float, float);
			void setPan(float);

			float position() const;

			bool playing() const;
			
			Track::Pointer track() const
				{ return _track; }
			
			unsigned int source() const;
			
			ET_DECLARE_EVENT1(finished, Player*)

        private:
			ET_DENY_COPY(Player)

			void init();
			void linkTrack(Track::Pointer);
			
			void setActualVolume(float);
            
			void handleProcessedBuffers();
			void handleProcessedSamples();
			
        private:
			friend class Manager;
			friend class StreamingThread;

		private:
			ET_DECLARE_PIMPL(Player, 32)
			
			Track::Pointer _track;
			FloatAnimator _volumeAnimator;
        };
	}
}
