/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/timers/animator.h>
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
			Player();
            Player(Track::Pointer track);
			
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
            PlayerPrivate* _private;
			Track::Pointer _track;
			
			FloatAnimator _volumeAnimator;
			float _volume = 1.0f;
        };
	}
}
