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
			std::list<Player::Pointer> playersList;
			std::list<Player::Pointer> playersToAdd;
			std::list<Player::Pointer> playersToRemove;
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

ThreadResult StreamingThread::main()
{
	while (running())
	{
		CriticalSectionScope scope(_private->csLock);
		
		for (auto& p : _private->playersToAdd)
			_private->playersList.push_back(p);

		for (auto& p : _private->playersToRemove)
			_private->playersList.remove(p);
		
		_private->playersToAdd.clear();
		_private->playersToRemove.clear();
		
		for (Player::Pointer& player : _private->playersList)
		{
			ALint processedBuffers = 0;
			ALint queuedBuffers = 0;
			alGetSourcei(player->source(), AL_BUFFERS_PROCESSED, &processedBuffers);
			alGetSourcei(player->source(), AL_BUFFERS_QUEUED, &queuedBuffers);
			player->loadNextBuffers(processedBuffers, queuedBuffers);
		}
		
		sleepMSec(100);
	}
	
	return 0;
}

void StreamingThread::addPlayer(Player::Pointer player)
{
	CriticalSectionScope scope(_private->csLock);
	
	auto i = std::find(_private->playersList.begin(), _private->playersList.end(), player);
	if (i == _private->playersList.end())
	{
		i = std::find(_private->playersToAdd.begin(), _private->playersToAdd.end(), player);
		if (i == _private->playersToAdd.end())
			_private->playersList.push_back(player);
	}
}

void StreamingThread::removePlayer(PlayerPointer player)
{
	CriticalSectionScope scope(_private->csLock);
	
	auto i = std::find(_private->playersList.begin(), _private->playersList.end(), player);
	if (i != _private->playersList.end())
	{
		i = std::find(_private->playersToRemove.begin(), _private->playersToRemove.end(), player);
		if (i == _private->playersToRemove.end())
			_private->playersToRemove.push_back(player);
	}
};
