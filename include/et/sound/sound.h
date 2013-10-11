/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/sound/formats.h>

namespace et
{
    namespace audio
    {
		class Player;
		class Manager;

        class TrackPrivate;
        class Track : public Object
        {
        public:
			ET_DECLARE_POINTER(Track)
            
		public:
			~Track();

			float duration() const;

			size_t channels() const;
			size_t sampleRate() const;
			size_t bitDepth() const;

		private:
            Track(const std::string& fileName);
			Track(Description::Pointer data);

			ET_SINGLETON_COPY_DENY(Track)

		private:
			void init(Description::Pointer data);
            
		private:
			friend class Player;
			friend class Manager;
            TrackPrivate* _private;
        };
        
        /*
         * Player
         */
        
        class PlayerPrivate;
        class Player : public Object
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

			void setVolume(float);
			void setPan(float);

			float position() const;

			bool playing() const;
			
			Track::Pointer currentTrack() const
				{ return _currentTrack; }

        private:
			Player();
            Player(Track::Pointer track);
			ET_SINGLETON_COPY_DENY(Player)

			void init();
			void linkTrack(Track::Pointer);
            
        private:
			friend class Manager;

		private:
            PlayerPrivate* _private;
			Track::Pointer _currentTrack;
        };
        
        /*
         * Manager
         */
        
        class ManagerPrivate;
        class Manager : public Singleton<Manager>
        {
        public:
			~Manager();

			Track::Pointer loadTrack(const std::string& fileName);
			Track::Pointer genTrack(Description::Pointer desc);

			Player::Pointer genPlayer(Track::Pointer track);
			Player::Pointer genPlayer();

        private:
            Manager();
			
			void nativePreInit();
			void nativeInit();
			void nativeRelease();
			void nativePostRelease();
           
        private:
            ET_SINGLETON_COPY_DENY(Manager)
            
        private:
            friend class ManagerPrivate;
            ManagerPrivate* _private;
        };

		inline Manager& manager()
			{ return Manager::instance(); }
    }
}