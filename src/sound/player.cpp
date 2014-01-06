/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/sound/sound.h>

namespace et
{
    namespace audio
    {
        class PlayerPrivate
        {
		public:     
			ALuint source = 0;
			bool playingLooped = false;
			int buffersProcessed = 0;
		};
    }
}

using namespace et;
using namespace et::audio;

extern ALCdevice* getSharedDevice();
extern ALCcontext* getSharedContext();

Player::Player() : 
	_private(new PlayerPrivate), _volumeAnimator(mainTimerPool())
{
	init();
}

Player::Player(Track::Pointer track) : 
	_private(new PlayerPrivate), _volumeAnimator(mainTimerPool())
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
	ET_CONNECT_EVENT(_volumeAnimator.updated, Player::onVolumeUpdated)

	alGenSources(1, &_private->source);
    checkOpenALError("alGenSources");

	alSourcef(_private->source, AL_PITCH, 1.0f);
    checkOpenALError("alSourcef(..., AL_PITCH, ...)");

	setVolume(1.0f);
	
	vec3 nullVector;
	alSourcefv(_private->source, AL_POSITION, nullVector.data());
    checkOpenALError("alSourcefv(..., AL_POSITION, ...)");
	
	alSourcefv(_private->source, AL_VELOCITY, nullVector.data());
    checkOpenALError("alSourcefv(..., AL_VELOCITY, ...)");
}

void Player::play(bool looped)
{
	int state = 0;
	alGetSourcei(_private->source, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING)
	{
	}
	else if (state == AL_PAUSED)
	{
		alSourcePlay(_private->source);
		checkOpenALError("alSourcePlay");
	}
	else if ((state == AL_INITIAL) || (state == AL_STOPPED))
	{
		int queued = 0;
		alGetSourcei(_private->source, AL_BUFFERS_QUEUED, &queued);
		while (queued--)
		{
			ALuint buffer = 0;
			alSourceUnqueueBuffers(_private->source, queued, &buffer);
			checkOpenALError("alSourceUnqueueBuffers");
		}
		alSourcei(_private->source, AL_BUFFER, 0);
		
		_private->buffersProcessed = 0;
		_private->playingLooped = looped;
		
		_track->preloadBuffers();
		
		if (_track->streamed())
		{
			alSourceQueueBuffers(_private->source, _track->actualBuffersCount(), _track->buffers());
			checkOpenALError("alSourceQueueBuffers(.., %d, %u)", _track->actualBuffersCount(), _track->buffers());
			alSourcei(_private->source, AL_LOOPING, AL_FALSE);
			checkOpenALError("alSourcei(..., AL_LOOPING, ...)");
		}
		else
		{
			alSourcei(_private->source, AL_BUFFER, _track->buffer());
			checkOpenALError("alSourcei(.., AL_BUFFER, ...)");
			alSourcei(_private->source, AL_LOOPING, looped ? AL_TRUE : AL_FALSE);
			checkOpenALError("alSourcei(..., AL_LOOPING, ...)");
		}
		
		alSourcePlay(_private->source);
		checkOpenALError("alSourcePlay");
		
		manager().streamingThread().addPlayer(this);
	}
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
	manager().streamingThread().removePlayer(this);
	
	alSourceStop(_private->source);
    checkOpenALError("alSourceStop");
	
	alSourcei(_private->source, AL_BUFFER, 0);
	checkOpenALError("alSourcei");
	
	if (_track.valid())
		_track->rewind();
	
	_private->playingLooped = false;
	_private->buffersProcessed = 0;
}

void Player::rewind()
{
    alSourceRewind(_private->source);
    checkOpenALError("alSourceRewind");
}

void Player::linkTrack(Track::Pointer track)
{
    stop();
	_track = track;
}

void Player::setVolume(float value, float duration)
{
	_volumeAnimator.cancelUpdates();
	
	if (duration == 0.0f)
	{
		setVolume(value);
	}
	else
	{
		_volumeAnimator.animate(&_volume, _volume, value, duration);
	}
}

float Player::position() const
{
	if (_track.invalid()) return 0.0f;

	float sampleOffset = 0.0f;
	alGetSourcef(_private->source, AL_SAMPLE_OFFSET, &sampleOffset);
	checkOpenALError("alGetSourcef(..., AL_SAMPLE_OFFSET, ");

	return sampleOffset / static_cast<float>(_track->sampleRate());
}

bool Player::playing() const
{
	if (_track.invalid()) return false;
	
	ALint state = 0;
	alGetSourcei(_private->source, AL_SOURCE_STATE, &state);
	
	return (state == AL_PLAYING);
}

void Player::setPan(float pan)
{
	if (_track.invalid()) return;
	
	if (_track->channels() > 1)
	{
		log::warning("Unable to set pan for stereo sound: %s", _track->origin().c_str());
	}
	else
	{
		alSource3f(_private->source, AL_POSITION, pan, 0.0f, 0.0f);
		checkOpenALError("alSource3f(..., AL_POSITION, ");
	}
}

unsigned int Player::source() const
{
	return _private->source;
}

void Player::handleProcessedBuffers()
{
	int processed = 0;
	alGetSourcei(_private->source, AL_BUFFERS_PROCESSED, &processed);
	checkOpenALError("alGetSourcei(%d, AL_BUFFERS_PROCESSED, %d)", _private->source, processed);
	
	if (processed > 0)
	{
		_private->buffersProcessed += processed;
		
		while (processed--)
		{
			ALuint buffer = 0;
			alSourceUnqueueBuffers(_private->source, 1, &buffer);
			checkOpenALError("alSourceUnqueueBuffers");
		}
		
		int remaining = 0;
		alGetSourcei(_private->source, AL_BUFFERS_QUEUED, &remaining);
		
		bool shouldLoadNextBuffer = _private->playingLooped ||
		((_private->buffersProcessed + remaining) < _track->totalBuffersCount());
		
		if (shouldLoadNextBuffer)
		{
			ALuint buffer = _track->loadNextBuffer();
			if (buffer > 0)
			{
				alSourceQueueBuffers(_private->source, 1, &buffer);
				checkOpenALError("alSourceQueueBuffers");
			}
		}
		else if (remaining == 0)
		{
			stop();
			finished.invokeInMainRunLoop(this);
		}
	}
}

void Player::onVolumeUpdated()
{
	setVolume(_volume);
}

void Player::setVolume(float v)
{
	_volume = etMax(0.0f, etMin(1.0f, v));
	alSourcef(_private->source, AL_GAIN, _volume);
	checkOpenALError("alSourcei(.., AL_GAIN, ...)");
}

void Player::handleProcessedSamples()
{
	int sampleOffset = 0;
	alGetSourcei(_private->source, AL_SAMPLE_OFFSET, &sampleOffset);
	checkOpenALError("alGetSourcei(%d, AL_SAMPLE_OFFSET, %d)", _private->source, sampleOffset);
	
	if ((sampleOffset == track()->samples()) && !_private->playingLooped)
	{
		stop();
		finished.invokeInMainRunLoop(this);
	}
}
