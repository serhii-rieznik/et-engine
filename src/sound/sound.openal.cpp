/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <iostream>
#include <assert.h>
#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/sound/openal.h>
#include <et/sound/sound.h>

namespace et
{
    namespace audio
    {
        class TrackPrivate
        {
		public:     
			Description::Pointer desc;
			TrackPrivate() : buffer(0) { }
			ALuint buffer;
        };
        
        class PlayerPrivate
        {
		public:     
			PlayerPrivate() : source(0) { }
			ALuint source;
		};

		class ManagerPrivate
		{
		public:
			ManagerPrivate() { }
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
void checkOpenALErrorEx(const char* caller, const char* file, const char* line, const char* tag);

#if (ET_DEBUG)
#   define checkOpenALError(tag) checkOpenALErrorEx(ET_CALL_FUNCTION, __FILE__, ET_TOCONSTCHAR(__LINE__), tag)
#else
#   define checkOpenALError(tag)
#endif

Manager::Manager() : _private(new ManagerPrivate)
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
	assert(success); (void)success;

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

Track::Pointer Manager::genTrack(Description::Pointer desc)
{
	return Track::Pointer(new Track(desc));
}

Player::Pointer Manager::genPlayer(Track::Pointer track)
{
	assert(track.valid());
	return Player::Pointer(new Player(track));
}

Player::Pointer Manager::genPlayer()
{
	return Player::Pointer(new Player);
}

/*
 * Track implementation
 */ 

Track::Track(const std::string& fileName) : _private(new TrackPrivate)
{
	init(loadFile(fileName));
}

Track::Track(Description::Pointer desc) : _private(new TrackPrivate)
{
	init(desc);
}

Track::~Track()
{
	alDeleteBuffers(1, &_private->buffer);
	delete _private;
}

float Track::duration() const
{
	return _private->desc->duration;
}

size_t Track::channels() const
{
	return _private->desc->channels;
}

size_t Track::sampleRate() const
{
	return _private->desc->sampleRate;
}

size_t Track::bitDepth() const
{
	return _private->desc->bitDepth;
}

void Track::init(Description::Pointer data)
{
	_private->desc = data;

	alGenBuffers(1, &_private->buffer);
    checkOpenALError("alGenBuffers");

	if (data.invalid()) return;

	size_t sampleSize = data->bitDepth * data->channels;
	size_t numSamples = data->data.dataSize() / sampleSize;
	size_t actualDataSize = numSamples * sampleSize;

	if (actualDataSize != data->data.dataSize())
	{
		size_t remain = data->data.dataSize() % sampleSize;
		std::cout << data->origin() << std::endl <<
			"\tincorrect audio data size: " << data->data.dataSize() << ", should be: " << actualDataSize << 
			" to fit " << numSamples << " samples (" << remain << " bytes remained)." <<  std::endl;
	}

	alBufferData(_private->buffer, static_cast<ALenum>(data->format), data->data.data(),
		static_cast<ALsizei>(actualDataSize), static_cast<ALsizei>(data->sampleRate));
	
    checkOpenALError("alBufferData");
	
	data->data.resize(0);
}

/*
 * Player imlementation
 */

Player::Player() : 
	_private(new PlayerPrivate)
{
	init();
}

Player::Player(Track::Pointer track) : 
	_private(new PlayerPrivate)
{
	init();
	linkTrack(track);
}

Player::~Player()
{
	stop();

	alDeleteSources(1, &_private->source);
    checkOpenALError("alDeleteSources");

	delete _private;
}

void Player::init()
{
	vec3 nullVector;

	alGenSources(1, &_private->source);
    checkOpenALError("alGenSources");

	alSourcef(_private->source, AL_PITCH, 1.0f);
    checkOpenALError("alSourcef(..., AL_PITCH, ...)");

	alSourcef(_private->source, AL_GAIN, 1.0f);
    checkOpenALError("alSourcef(..., AL_GAIN, ...)");

	alSourcefv(_private->source, AL_POSITION, nullVector.data());
    checkOpenALError("alSourcefv(..., AL_POSITION, ...)");

	alSourcefv(_private->source, AL_VELOCITY, nullVector.data());
    checkOpenALError("alSourcefv(..., AL_VELOCITY, ...)");
}

void Player::play(bool looped)
{
	alSourcei(_private->source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE);
    checkOpenALError("alSourcei(..., AL_LOOPING, ...)");
	
	alSourcePlay(_private->source);
    checkOpenALError("alSourcePlay");
}

void Player::play(Track::Pointer track, bool looped)
{
	linkTrack(track);
	play(looped);
}

void Player::pause()
{
	alSourcePause(_private->source);
    checkOpenALError("alSourcePause");
}

void Player::stop()
{
	alSourceStop(_private->source);
    checkOpenALError("alSourceStop");
}

void Player::rewind()
{
    alSourceRewind(_private->source);
    checkOpenALError("alSourceRewind");
}

void Player::linkTrack(Track::Pointer track)
{
	if (_currentTrack == track) return;

    stop();
    
	_currentTrack = track;
	alSourcei(_private->source, AL_BUFFER, track->_private->buffer);
    checkOpenALError("alSourcei(.., AL_BUFFER, ...)");
}

void Player::setVolume(float value)
{
	alSourcef(_private->source, AL_GAIN, value);
    checkOpenALError("alSourcei(.., AL_GAIN, ...)");
}

float Player::position() const
{
	if (_currentTrack.invalid()) return 0.0f;

	float sampleOffset = 0.0f;
	alGetSourcef(_private->source, AL_SAMPLE_OFFSET, &sampleOffset);
	checkOpenALError("alGetSourcef(..., AL_SAMPLE_OFFSET, ");

	return sampleOffset / static_cast<float>(_currentTrack->sampleRate());
}

bool Player::playing() const
{
	if (_currentTrack.invalid()) return false;
	
	ALint state = 0;
	alGetSourcei(_private->source, AL_SOURCE_STATE, &state);
	
	return (state == AL_PLAYING);
}

void Player::setPan(float pan)
{
	if (_currentTrack.invalid()) return;
	if (_currentTrack->channels() > 1)
	{
		std::cout << "Unable to set pan for stereo sound: " << _currentTrack->_private->desc->origin() << std::endl;
		return;
	}

	alSource3f(_private->source, AL_POSITION, pan, 0.0f, 0.0f);
	checkOpenALError("alSource3f(..., AL_POSITION, ");
}

/*
 * Service functions
 */
extern ALCdevice* getSharedDevice()
	{ return sharedDevice; }

extern ALCcontext* getSharedContext()
	{ return sharedContext; }

void checkOpenALErrorEx(const char* caller, const char* sourceFile, const char* lineNumber, const char* tag)
{
	if (sharedDevice == nullptr) return;
	
	ALenum error = alcGetError(sharedDevice);
	if (error != ALC_NO_ERROR)
	{
		const char* message = alcGetString(sharedDevice, error);
		log::error("OpenAL ALC error: %s\n%s [%s, %s]: %s\n", message, sourceFile, lineNumber, caller, tag);
        fflush(stdout);
	}
    
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		const char* message = alGetString(error);
        printf("OpenAL error: %s\n%s[%s]: %s\n", message, sourceFile, lineNumber, tag);
        fflush(stdout);
	}
}
