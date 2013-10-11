/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <AudioToolbox/AudioToolbox.h>
#include <et/sound/sound.h>

using namespace et;
using namespace audio;

extern ALCdevice* getSharedDevice();
extern ALCcontext* getSharedContext();

void etInterruptListener(void*, UInt32 inInterruptionState);

void Manager::nativePreInit()
{
}

void Manager::nativeInit()
{
	AudioSessionInitialize(nil, nil, etInterruptListener, nil);
	AudioSessionSetActive(true);
}

void Manager::nativeRelease()
{
	AudioSessionSetActive(false);
}

void Manager::nativePostRelease()
{
}

/*
 * Service functions
 */
void etInterruptListener(void*, UInt32 inInterruptionState)
{
	if (inInterruptionState == kAudioSessionBeginInterruption)
	{
		AudioSessionSetActive(false);
		alcMakeContextCurrent(0);
		alcSuspendContext(getSharedContext());
	}
	else if (inInterruptionState == kAudioSessionEndInterruption)
	{
		int restart = 0;
		OSStatus error = AudioSessionSetActive(true);
		while ((error != 0) && (++restart > 10))
		{
			sleep(1);
			error = AudioSessionSetActive(true);
		}
		
		if (error == 0)
		{
			alcMakeContextCurrent(getSharedContext());
			alcProcessContext(getSharedContext());
		}
	}
}
