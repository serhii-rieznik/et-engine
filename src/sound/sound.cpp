/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/sound/sound.h>

namespace et
{
    namespace audio
    {
		class ManagerPrivate
		{
		};
    }
}

using namespace et;
using namespace et::audio;

/*
 * Manager implementation
 */

static ALCdevice* sharedDevice = nullptr;
static ALCcontext* sharedContext = nullptr;

ALCdevice* getSharedDevice();
ALCcontext* getSharedContext();

Manager::Manager() :
	_private(new ManagerPrivate)
{
	nativePreInit();
	
	const char* defaultDeviceSpecifier = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	log::info("[Audio::Manager] Default OpenAL device is `%s`", defaultDeviceSpecifier);

	sharedDevice = alcOpenDevice(defaultDeviceSpecifier);
	if (alcOpenDevice == nullptr)
	{
		log::error("Unable to initialize OpenAL device");
		return;
	}
	
	sharedContext = alcCreateContext(sharedDevice, nullptr);
	if (sharedContext == nullptr)
	{
		log::error("Unable to initialize OpenAL context");
		return;
	}

	nativeInit();
    
	ALboolean success = alcMakeContextCurrent(sharedContext);
	ET_ASSERT(success); (void)success;

	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	checkOpenALError("alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED)");

	vec3 nullVector;
	alListenerfv(AL_POSITION, nullVector.data());
	checkOpenALError("alListenerfv(AL_POSITION, ...");

    alListenerfv(AL_VELOCITY, nullVector.data());
	checkOpenALError("alListenerfv(AL_VELOCITY, ...");

	float orientation[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	alListenerfv(AL_ORIENTATION, orientation);
	checkOpenALError("alListenerfv(AL_ORIENTATION, ...");
}

Manager::~Manager()
{
	_streamingThread.stop();
	_streamingThread.waitForTermination();
	
	if (sharedDevice == nullptr) return;
	
	nativeRelease();
	alcMakeContextCurrent(0);
	alcDestroyContext(sharedContext);
	alcCloseDevice(sharedDevice);
	delete _private;
    
    sharedDevice = nullptr;
	
	nativePostRelease();
}

Track::Pointer Manager::loadTrack(const std::string& fileName)
{
	return Track::Pointer(new Track(fileName));
}

Player::Pointer Manager::genPlayer(Track::Pointer track)
{
	ET_ASSERT(track.valid());
	return Player::Pointer(new Player(track));
}

Player::Pointer Manager::genPlayer()
{
	return Player::Pointer(new Player);
}

/*
 * Service functions
 */
extern ALCdevice* getSharedDevice()
	{ return sharedDevice; }

extern ALCcontext* getSharedContext()
	{ return sharedContext; }

void et::audio::checkOpenALErrorEx(const char* caller, const char* file, const char* line, const char* tag, ...)
{
	if (sharedDevice == nullptr) return;
	
	ALenum alError = alGetError();
	ALenum alcError = alcGetError(sharedDevice);
	
	if ((alcError != ALC_NO_ERROR) || (alError != AL_NO_ERROR))
	{
		char buffer[1024] = { };
		
		va_list args;
		va_start(args, tag);
		vsnprintf(buffer, sizeof(buffer), tag, args);
		va_end(args);
		
		if (alcError != AL_NO_ERROR)
		{
			log::error("OpenAL ALC error: %s\n%s [%s, %s]: %s", alcGetString(sharedDevice, alcError),
				file, line, caller, buffer);
		}
		
		if (alError != AL_NO_ERROR)
			log::error("OpenAL error: %s\n%s[%s]: %s", alGetString(alError), file, line, buffer);
	}
}

size_t et::audio::openALFormatFromChannelsAndBitDepth(size_t numChannels, size_t bitDepth)
{
	if (numChannels == 1)
	{
		if (bitDepth == 8)
		{
			return AL_FORMAT_MONO8;
		}
		else if (bitDepth == 16)
		{
			return AL_FORMAT_MONO16;
		}
		else
		{
			ET_ASSERT(false && "Unsupported format in WAV file");
		}
	}
	else if (numChannels == 2)
	{
		if (bitDepth == 8)
		{
			return AL_FORMAT_STEREO8;
		}
		else if (bitDepth == 16)
		{
			return AL_FORMAT_STEREO16;
		}
		else
		{
			ET_ASSERT(false && "Unsupported audio format.");
		}
	}
	else
	{
		ET_ASSERT(false && "Unsupported number of channels in audio.");
	}
	
	return 0;
}
