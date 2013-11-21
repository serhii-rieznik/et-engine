/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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
			
			void addPlayer(PlayerPointer);
			void removePlayer(PlayerPointer);
			
		private:
			ThreadResult main();
			
		private:
			StreamingThreadPrivate* _private;
		};
	}
}