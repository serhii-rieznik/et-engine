/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/sound/streamingthread.h>
#include <et/sound/player.h>

namespace et
{
    namespace audio
    {
        class ManagerPrivate;
        class Manager : public Singleton<Manager>
        {
        public:
			Track::Pointer loadTrack(const std::string& fileName);

			Player::Pointer genPlayer(Track::Pointer track);
			Player::Pointer genPlayer();

        private:
            Manager();
			~Manager();
			
			void nativePreInit();
			void nativeInit();
			void nativeRelease();
			void nativePostRelease();
			
			StreamingThread& streamingThread()
				{ return _streamingThread; }
           
        private:
            ET_SINGLETON_COPY_DENY(Manager)
            
        private:
            friend class ManagerPrivate;
			friend class Player;
			
			ET_DECLARE_PIMPL(Manager, 32)
			
			StreamingThread _streamingThread;
        };

		inline Manager& manager()
			{ return Manager::instance(); }
    }
}
