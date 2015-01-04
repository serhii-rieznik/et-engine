/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>
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

			std::list<Player::Pointer> playersList;
			std::list<Player::Pointer> playersToAdd;
			std::list<Player::Pointer> playersToRemove;
		};
	}
}

using namespace et;
using namespace et::audio;

StreamingThread::StreamingThread() :
	Thread(false)
{
	ET_PIMPL_INIT(StreamingThread)
	run();
}

void StreamingThread::release()
{
	ET_PIMPL_FINALIZE(StreamingThread)
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

		for (auto player : _private->playersList)
		{
			if (player->track().valid() && player->track()->streamed())
				player->handleProcessedBuffers();
			
			player->handleProcessedSamples();
		}
		
		sleepMSec(50);
	}

	CriticalSectionScope scope(_private->csLock);
	_private->playersToAdd.clear();
	_private->playersToRemove.clear();
	_private->playersList.clear();
	
	return 0;
}

void StreamingThread::addPlayer(Player::Pointer player)
{
	ET_ASSERT(_private != nullptr)
	
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

void StreamingThread::removePlayer(Player::Pointer player)
{
	if (_private == nullptr) return;
	
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
