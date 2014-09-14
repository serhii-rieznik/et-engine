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
		class StreamingThreadPrivate;
		class StreamingThread : public Thread
		{
		public:
			StreamingThread();
			~StreamingThread();
			
			void addPlayer(Player*);
			void removePlayer(Player*);
			
		private:
			ThreadResult main();
			
		private:
			StreamingThreadPrivate* _private;
		};
	}
}
