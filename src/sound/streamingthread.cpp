/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/threading/criticalsection.h>
#include <et/sound/sound.h>

namespace et
{
	namespace audio
	{
		class StreamingThreadPrivate
		{
		public:
			CriticalSection csLock;

			std::list<Player*> playersList;
			std::list<Player*> playersToAdd;
			std::list<Player*> playersToRemove;
		};
	}
}

using namespace et;
using namespace et::audio;

StreamingThread::StreamingThread() :
	Thread(false), _private(new StreamingThreadPrivate)
{
	run();
}

StreamingThread::~StreamingThread()
{
	{
		CriticalSectionScope scope(_private->csLock);
	}
	delete _private;
}

ThreadResult StreamingThread::main()
{
	while (running())
	{
		{
			CriticalSectionScope scope(_private->csLock);

			for (auto& p : _private->playersToAdd)
				_private->playersList.push_back(p);
			for (auto& p : _private->playersToRemove)
				_private->playersList.remove(p);

			_private->playersToAdd.clear();
			_private->playersToRemove.clear();
		}

		for (Player* player : _private->playersList)
		{
			if (player->track().valid() && player->track()->streamed())
				player->handleProcessedBuffers();
			
			player->handleProcessedSamples();
		}
		
		sleepMSec(50);
	}
	
	return 0;
}

void StreamingThread::addPlayer(Player* player)
{
	CriticalSectionScope scope(_private->csLock);
	
	auto i = std::find(_private->playersList.begin(), _private->playersList.end(), player);
	if (i == _private->playersList.end())
	{
		i = std::find(_private->playersToAdd.begin(), _private->playersToAdd.end(), player);
		if (i == _private->playersToAdd.end())
			_private->playersToAdd.push_back(player);
		
		i = std::find(_private->playersToRemove.begin(), _private->playersToRemove.end(), player);
		if (i != _private->playersToRemove.end())
			_private->playersToRemove.erase(i);
	}
}

void StreamingThread::removePlayer(Player* player)
{
	CriticalSectionScope scope(_private->csLock);
	
	auto i = std::find(_private->playersList.begin(), _private->playersList.end(), player);
	if (i != _private->playersList.end())
	{
		i = std::find(_private->playersToRemove.begin(), _private->playersToRemove.end(), player);
		if (i == _private->playersToRemove.end())
			_private->playersToRemove.push_back(player);
		
		i = std::find(_private->playersToAdd.begin(), _private->playersToAdd.end(), player);
		if (i != _private->playersToAdd.end())
			_private->playersToAdd.erase(i);
	}
};
