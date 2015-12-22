/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/sound/sound.h>

#if (ET_PLATFORM_IOS)

#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MPMusicPlayerController.h>

#include <et/platform-ios/ios.h>

using namespace et;
using namespace audio;

extern ALCdevice* getSharedDevice();
extern ALCcontext* getSharedContext();

void etInterruptListener(void*, UInt32 inInterruptionState);
bool activateAudioSession();
bool deactivateAudioSession();

void Manager::nativePreInit()
{
	NSError* err = nil;
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&err];
	
	if (err != nil)
		NSLog(@"Unable to set audio session category: %@", err);
}

void Manager::nativeInit()
{
	activateAudioSession();
}

void Manager::nativeRelease()
{
	deactivateAudioSession();
}

void Manager::nativePostRelease()
{
}

/*
 * Service functions
 */
bool activateAudioSession()
{
	NSError* err = nil;
	BOOL success = [[AVAudioSession sharedInstance] setActive:YES error:&err];
	
	if (err != nil)
		NSLog(@"Unable to activate audio session: %@", err);
	
	return success ? true : false;
}

bool deactivateAudioSession()
{
	NSError* err = nil;
	BOOL success = [[AVAudioSession sharedInstance] setActive:NO error:&err];
	
	if (err != nil)
		NSLog(@"Unable to deactivate audio session: %@", err);
	
	return success ? true : false;
}

void etInterruptListener(void*, UInt32 inInterruptionState)
{
	if (inInterruptionState == kAudioSessionBeginInterruption)
	{
		deactivateAudioSession();
		alcMakeContextCurrent(nullptr);
		alcSuspendContext(getSharedContext());
	}
	else if (inInterruptionState == kAudioSessionEndInterruption)
	{
		int restart = 0;
		bool success = activateAudioSession();
		while (!success && (++restart > 10))
		{
			sleep(1);
			success = activateAudioSession();
		}
		
		if (success)
		{
			alcMakeContextCurrent(getSharedContext());
			alcProcessContext(getSharedContext());
		}
	}
}

bool et::ios::musicIsPlaying()
{
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_8_0)
	auto player = [MPMusicPlayerController systemMusicPlayer];
#else
	auto player = [MPMusicPlayerController iPodMusicPlayer];
#endif
	
	return ([player nowPlayingItem] == nil) ? false :
		([player playbackState] == MPMusicPlaybackStatePlaying);
}

#endif // ET_PLATFORM_IOS
