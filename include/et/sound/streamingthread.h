/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/sound/openal.h>
#include <et/threading/thread.h>

namespace et
{
    namespace audio
    {
		class Player;
		typedef IntrusivePtr<Player> PlayerPointer;
		
		class StreamingThreadPrivate;
		class StreamingThread : public Thread
		{
		public:
			StreamingThread();
			
			void release();
			
			void addPlayer(PlayerPointer);
			void removePlayer(PlayerPointer);
			
		private:
			ThreadResult main();
			
		private:
			ET_DECLARE_PIMPL(StreamingThread, 256)
		};
	}
}
