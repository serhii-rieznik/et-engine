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
			if (player->track()->streamed())
			{
				ALint processedBuffers = 0;
				alGetSourcei(player->source(), AL_BUFFERS_PROCESSED, &processedBuffers);
				
				if (processedBuffers > 0)
					player->buffersProcessed(processedBuffers);
			}
			
			int sampleOffset = 0;
			alGetSourcei(player->source(), AL_SAMPLE_OFFSET, &sampleOffset);
			player->samplesProcessed(sampleOffset);
		}
		
		sleepMSec(50);
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
			_private->playersToAdd.push_back(player);
		
		i = std::find(_private->playersToRemove.begin(), _private->playersToRemove.end(), player);
		if (i != _private->playersToRemove.end())
			_private->playersToRemove.erase(i);
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
		
		i = std::find(_private->playersToAdd.begin(), _private->playersToAdd.end(), player);
		if (i != _private->playersToAdd.end())
			_private->playersToAdd.erase(i);
	}
};
