/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <et/core/containers.h>
#include <et/sound/sound.h>

namespace et
{
    namespace audio
    {
		const int BuffersCount = 3;
		
        class TrackPrivate
        {
		public:
			TrackPrivate();
			~TrackPrivate();
			
			void loadWAVE();
			void loadOGG();

			void rewind();
			void rewindPCM();
			void rewindOGG();
			
			bool fillNextBuffer();
			bool fillNextPCMBuffer();
			bool fillNextOGGBuffer();
			
			enum SourceFormat
			{
				SourceFormat_Undefined,
				SourceFormat_PCM,
				SourceFormat_OGG,
			};
			
		public:
			Track* owner = nullptr;
			
			InputStream::Pointer stream;
			
			float duration = 0.0f;
			
			size_t channels = 0;
			size_t bitDepth = 0;
			size_t format = 0;
			size_t sampleSize = 0;
			size_t numSamples = 0;
			
			size_t pcmDataSize = 0;
			size_t pcmStartPosition = 0;
			size_t pcmReadOffset = 0;
			size_t pcmBufferSize = 0;
			
			ALsizei sampleRate = 0;
			ALuint buffers[BuffersCount] = { };
			
			int bufferIndex = 0;
			int buffersCount = 0;
			
			OggVorbis_File oggFile = { };
			ov_callbacks oggCallbacks = { };
			size_t oggStartPosition = 0;
			
			SourceFormat sourceFormat = SourceFormat_Undefined;
        };
    }
}

using namespace et;
using namespace et::audio;

void checkOGGError(long, const char*);

/*
 * Track implementation
 */ 

Track::Track(const std::string& fileName) :
	_private(new TrackPrivate)
{
	_private->owner = this;
	setOrigin(fileName);
		
	_private->stream = InputStream::Pointer::create(fileName, StreamMode_Binary);
	if (_private->stream.invalid())
	{
		log::error("Unable to load file %s", fileName.c_str());
		return;
	}
	
	size_t dotPos = fileName.rfind('.');
	std::string ext = fileName.substr(dotPos);
	lowercase(ext);
	
	if (ext == ".wav")
	{
		_private->loadWAVE();
	}
	else if ((ext == ".aif") || (ext == ".aiff") || (ext == ".aifc"))
	{
		
	}
	else if (ext == ".ogg")
	{
		_private->loadOGG();
	}
}

Track::~Track()
{
	delete _private;
}

float Track::duration() const
{
	return _private->duration;
}

size_t Track::channels() const
{
	return _private->channels;
}

size_t Track::sampleRate() const
{
	return _private->sampleRate;
}

size_t Track::bitDepth() const
{
	return _private->bitDepth;
}

unsigned int Track::buffer() const
{
	return _private->buffers[0];
}

unsigned int* Track::buffers() const
{
	return _private->buffers;
}

int Track::buffersCount() const
{
	return _private->buffersCount;
}

bool Track::streamed() const
{
	return _private->buffersCount > 1;
}

unsigned int Track::loadNextBuffer()
{
	int bufferToLoad = _private->bufferIndex;
	return _private->fillNextBuffer() ? _private->buffers[bufferToLoad] : 0;
}

void Track::rewind()
{
	_private->bufferIndex = 0;
	_private->rewind();
}

void Track::preloadBuffers()
{
	for (size_t i = 0; i < _private->buffersCount; ++i)
		_private->fillNextBuffer();
}

/*
 *
 * Track private
 *
 */
#pragma pack(1)

union ChunkIdentifier
{
	char cID[4];
	unsigned char ucID[4];
	uint32_t nID;
};

struct WAVFileChunk
{
	ChunkIdentifier id;
	uint32_t chunkSize;
	char format[4];
};

struct AudioFileChunk
{
	ChunkIdentifier id;
	uint32_t size;
};

struct WAVFormatChunk
{
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};

#pragma pack()

const uint32_t WAVFileChunkID = ET_COMPOSE_UINT32_INVERTED('R', 'I', 'F', 'F');
const uint32_t WAVDataChunkID = ET_COMPOSE_UINT32_INVERTED('d', 'a', 't', 'a');
const uint32_t WAVFormatChunkID = ET_COMPOSE_UINT32_INVERTED('f', 'm', 't', ' ');

TrackPrivate::TrackPrivate()
{
	oggCallbacks.read_func = [](void* ptr, size_t size, size_t nmemb, void* datasource) -> size_t
	{
		InputStream* stream = reinterpret_cast<InputStream*>(datasource);
		
		if (stream->valid())
			stream->stream().read(reinterpret_cast<char*>(ptr), size * nmemb);
		
		return stream->valid() ? stream->stream().gcount() : 0;
	};
	
	
	oggCallbacks.seek_func = [](void* datasource, ogg_int64_t p1, int p2) -> int
	{
		InputStream* stream = reinterpret_cast<InputStream*>(datasource);
		
		if (stream->valid())
			stream->stream().seekg(p1, static_cast<std::ios::seekdir>(p2));
		
		return stream->valid() ? 0 : -1;
	};
	
	oggCallbacks.tell_func = [](void* datasource) -> long
	{
		InputStream* stream = reinterpret_cast<InputStream*>(datasource);
		return static_cast<long>(stream->stream().tellg());
	};
}

TrackPrivate::~TrackPrivate()
{
	if (sourceFormat == SourceFormat_OGG)
		ov_clear(&oggFile);
	
	alDeleteBuffers(buffersCount, buffers);
	checkOpenALError("alDeleteBuffers(%d, ...", buffersCount);
}

bool TrackPrivate::fillNextBuffer()
{
	switch (sourceFormat)
	{
		case SourceFormat_PCM:
			return fillNextPCMBuffer();
		case SourceFormat_OGG:
			return fillNextOGGBuffer();
		default:
			return false;
	}
}

void TrackPrivate::rewind()
{
	switch (sourceFormat)
	{
		case SourceFormat_PCM:
			rewindPCM();
			break;
			
		case SourceFormat_OGG:
			rewindOGG();
			break;
			
		default:
			break;
	}
}

/*
 * WAVE stuff
 */
void TrackPrivate::loadWAVE()
{
	auto& inStream = stream->stream();
	
	WAVFileChunk fileChunk = { };
	inStream.read(reinterpret_cast<char*>(&fileChunk), sizeof(fileChunk));
	
	if (fileChunk.id.nID != WAVFileChunkID) return;
	
	AudioFileChunk chunk = { };
	inStream.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
	if (chunk.id.nID != WAVFormatChunkID)
	{
		log::error("%s is not a WAV file", owner->origin().c_str());
		return;
	}
	
	WAVFormatChunk fmt = { };
	inStream.read(reinterpret_cast<char*>(&fmt), sizeof(fmt));
	
	do
	{
		inStream.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
		if (chunk.id.nID != WAVDataChunkID)
			inStream.seekg(chunk.size, std::ios::cur);
	}
	while (chunk.id.nID != WAVDataChunkID);
	
	sampleRate = fmt.sampleRate;
	channels = fmt.numChannels;
	bitDepth = fmt.bitsPerSample;
	format = openALFormatFromChannelsAndBitDepth(channels, bitDepth);
	
	size_t oneSecondSize = sampleRate * channels * bitDepth / 8;
	
	duration = static_cast<float>(chunk.size) / static_cast<float>(oneSecondSize);
	sampleSize = bitDepth * channels;
	numSamples = chunk.size / sampleSize;
	
	pcmBufferSize = 3 * oneSecondSize;
	pcmDataSize = numSamples * sampleSize;
	pcmStartPosition = static_cast<size_t>(inStream.tellg());
	pcmReadOffset = 0;

	buffersCount = etMin(BuffersCount, static_cast<int>(1 + pcmDataSize / pcmBufferSize));
	alGenBuffers(buffersCount, buffers);
	checkOpenALError("alGenBuffers(%d, ...)", buffersCount);
	
	sourceFormat = SourceFormat_PCM;
}

void TrackPrivate::rewindPCM()
{
	auto& inStream = stream->stream();
	inStream.clear();
	inStream.seekg(pcmStartPosition, std::ios::beg);
	pcmReadOffset = 0;
}

bool TrackPrivate::fillNextPCMBuffer()
{
	BinaryDataStorage data(pcmBufferSize, 0);
	auto& inStream = stream->stream();
	inStream.read(data.binary(), etMin(pcmBufferSize, pcmDataSize - pcmReadOffset));
	pcmReadOffset += inStream.gcount();
	
	ALsizei bytesRead = static_cast<ALsizei>(inStream.gcount());
	if (bytesRead > 0)
	{
		alBufferData(buffers[bufferIndex], static_cast<ALenum>(format), data.data(), bytesRead, sampleRate);
		checkOpenALError("alBufferData");
		bufferIndex = (bufferIndex + 1) % buffersCount;
	}
	
	if ((pcmReadOffset >= pcmDataSize) || inStream.eof())
		rewindPCM();
	
	return (bytesRead > 0);
}

/*
 * OGG stuff
 */
void TrackPrivate::loadOGG()
{
	int result = ov_open_callbacks(stream.ptr(), &oggFile, nullptr, -1, oggCallbacks);
	if (result < 0)
	{
		checkOGGError(result, "ov_open_callbacks");
		return;
	}

	int streamId = -1;
	vorbis_info* info = ov_info(&oggFile, streamId);
	
	bitDepth = 16;
	sampleRate = static_cast<ALsizei>(info->rate);
	channels = info->channels;
	
	format = openALFormatFromChannelsAndBitDepth(channels, bitDepth);
	
	size_t oneSecondSize = sampleRate * channels * bitDepth / 8;
	size_t computedDataSize = static_cast<size_t>(sizeof(float) * ov_pcm_total(&oggFile, streamId));
	
	sampleSize = bitDepth * channels;
	numSamples = computedDataSize / sampleSize;
	pcmDataSize = numSamples * sampleSize;
	pcmBufferSize = 3 * oneSecondSize;
	pcmStartPosition = static_cast<size_t>(ov_pcm_tell(&oggFile));
	oggStartPosition = static_cast<size_t>(ov_raw_tell(&oggFile));
	pcmReadOffset = 0;
	
	duration = static_cast<float>(pcmDataSize) / static_cast<float>(oneSecondSize);
	
	buffersCount = etMin(BuffersCount, static_cast<int>(1 + pcmDataSize / pcmBufferSize));
	alGenBuffers(buffersCount, buffers);
	checkOpenALError("alGenBuffers(%d, ...)", buffersCount);
	
	sourceFormat = SourceFormat_OGG;
}

bool TrackPrivate::fillNextOGGBuffer()
{
	BinaryDataStorage data(pcmBufferSize, 0);
	auto& inStream = stream->stream();
	
	long bytesRead = 0;
	while (bytesRead < pcmBufferSize)
	{
		int section = -1;
		long lastRead = ov_read(&oggFile, data.binary() + bytesRead, static_cast<int>(pcmBufferSize - bytesRead), 0, 2, 1, &section);
		
		if (lastRead > 0)
		{
			bytesRead += lastRead;
		}
		else if (lastRead < 0)
		{
			checkOGGError(lastRead, "ov_read");
			break;
		}
		else
		{
			break;
		}
	}
		
	pcmReadOffset += bytesRead;
	if (bytesRead > 0)
	{
		alBufferData(buffers[bufferIndex], static_cast<ALenum>(format), data.data(),
			static_cast<ALsizei>(bytesRead), sampleRate);
		
		checkOpenALError("alBufferData");
		
		bufferIndex = (bufferIndex + 1) % buffersCount;
	}
	
	if ((pcmReadOffset >= pcmDataSize) || inStream.eof())
		rewindOGG();
	
	return (bytesRead > 0);
}

void TrackPrivate::rewindOGG()
{
	stream->stream().clear();

	if (ov_seekable(&oggFile))
	{
		long result = ov_raw_seek(&oggFile, oggStartPosition);
		checkOGGError(result, "ov_raw_seek");
	}
	else
	{
		log::error("Unable to rewind OGG file.");
	}
	
	pcmReadOffset = 0;
}


/*
 * Service functions
 */
#define CASEOF(A) case A: { log::error("OGG error %s at %s", #A, tag); return; }
void checkOGGError(long code, const char* tag)
{
	switch (code)
	{
		case 0:
			return;
			
		CASEOF(OV_FALSE)
		CASEOF(OV_EOF)
		CASEOF(OV_HOLE)
		CASEOF(OV_EREAD)
		CASEOF(OV_EFAULT)
		CASEOF(OV_EIMPL)
		CASEOF(OV_EINVAL)
		CASEOF(OV_ENOTVORBIS)
		CASEOF(OV_EBADHEADER)
		CASEOF(OV_EVERSION)
		CASEOF(OV_ENOTAUDIO)
		CASEOF(OV_EBADPACKET)
		CASEOF(OV_EBADLINK)
		CASEOF(OV_ENOSEEK)
			
		default:
			log::error("Unknown OGG error: %ld at %s", code, tag);
			break;
	}
}