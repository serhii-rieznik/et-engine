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
		const int BufferDuration = 4;
		
        class TrackPrivate
        {
		public:
			TrackPrivate(const std::string& filename);
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
			const std::string& _filename;
			
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
			ALuint buffers[BuffersCount];
			
			int bufferIndex = 0;
			int buffersCount = 0;
			int totalBuffers = 0;
			
			OggVorbis_File oggFile;
			ov_callbacks oggCallbacks;
			size_t oggStartPosition = 0;
			
			SourceFormat sourceFormat = SourceFormat_Undefined;
        };
    }
}

using namespace et;
using namespace et::audio;

void checkOGGError(long, const char*, const char* filename);

/*
 * Track implementation
 */ 

Track::Track(const std::string& fileName) :
	_private(new TrackPrivate(fileName))
{
	setName(fileName);
	setOrigin(fileName);
	
	_private->owner = this;
		
	_private->stream = InputStream::Pointer::create(fileName, StreamMode_Binary);
	if (_private->stream->invalid())
	{
		log::error("Unable to load file %s", fileName.c_str());
	}
	else
	{
		size_t dotPos = fileName.rfind('.');
		std::string ext = fileName.substr(dotPos);
		lowercase(ext);
		
		if (ext == ".wav")
		{
			_private->loadWAVE();
		}
		else if (ext == ".ogg")
		{
			_private->loadOGG();
		}
		else
		{
			log::error("Unsupported sound file extension %s", ext.c_str());
		}
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

size_t Track::samples() const
{
	return _private->numSamples;
}

int Track::totalBuffersCount() const
{
	return _private->totalBuffers;
}

int Track::actualBuffersCount() const
{
	return _private->buffersCount;
}

unsigned int Track::buffer() const
{
	return _private->buffers[0];
}

unsigned int* Track::buffers() const
{
	return _private->buffers;
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
	for (int i = 0; i < _private->buffersCount; ++i)
		_private->fillNextBuffer();
}

/*
 *
 * Track private
 *
 */

extern ALCdevice* getSharedDevice();
extern ALCcontext* getSharedContext();

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
	uint32_t format;
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

const uint32_t WAVFileChunkID = ET_COMPOSE_UINT32_INVERTED('R', 'I', 'F', 'F');
const uint32_t WAVDataChunkID = ET_COMPOSE_UINT32_INVERTED('d', 'a', 't', 'a');
const uint32_t WAVFormatChunkID = ET_COMPOSE_UINT32_INVERTED('f', 'm', 't', ' ');

TrackPrivate::TrackPrivate(const std::string& filename) :
	_filename(filename)
{
	etFillMemory(&buffers, 0, sizeof(buffers));
	etFillMemory(&oggFile, 0, sizeof(oggFile));
	etFillMemory(&oggCallbacks, 0, sizeof(oggCallbacks));

	oggCallbacks.read_func = [](void* ptr, size_t size, size_t nmemb, void* datasource) -> size_t
	{
		InputStream* stream = reinterpret_cast<InputStream*>(datasource);
		
		if (stream->valid())
			stream->stream().read(reinterpret_cast<char*>(ptr), size * nmemb);
		
		return static_cast<size_t>(stream->valid() ? stream->stream().gcount() : 0);
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
	do
	{
		inStream.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
		if (chunk.id.nID != WAVFormatChunkID)
			inStream.seekg(chunk.size, std::ios::cur);
	}
	while ((chunk.id.nID != WAVFormatChunkID) && !inStream.eof());
	
	if (inStream.eof())
	{
		log::error("%s is not a WAV file", owner->origin().c_str());
		return;
	}
	
	WAVFormatChunk fmt = { };
	inStream.read(reinterpret_cast<char*>(&fmt), sizeof(fmt));
	inStream.seekg(chunk.size - sizeof(fmt), std::ios::cur);
	
	do
	{
		inStream.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
		if (chunk.id.nID != WAVDataChunkID)
			inStream.seekg(chunk.size, std::ios::cur);
	}
	while ((chunk.id.nID != WAVDataChunkID) && !inStream.eof());
	
	if (inStream.eof())
	{
		log::error("%s is invalid WAV file", owner->origin().c_str());
		return;
	}
	
	sampleRate = fmt.sampleRate;
	channels = fmt.numChannels;
	bitDepth = fmt.bitsPerSample;
	format = openALFormatFromChannelsAndBitDepth(channels, bitDepth);
	sampleSize = channels * bitDepth / 8;
	
	size_t oneSecondSize = sampleRate * sampleSize;
	
	duration = static_cast<float>(chunk.size) / static_cast<float>(oneSecondSize);
	numSamples = chunk.size / sampleSize;
	
	pcmBufferSize = BufferDuration * oneSecondSize;
	pcmDataSize = numSamples * sampleSize;
	pcmStartPosition = static_cast<size_t>(inStream.tellg());
	pcmReadOffset = 0;
	totalBuffers = static_cast<int>(1 + pcmDataSize / pcmBufferSize);
	buffersCount = etMin(BuffersCount, totalBuffers);
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
	checkOpenALError("fillNextPCMBuffer");
	
	BinaryDataStorage data(pcmBufferSize, 0);
	auto& inStream = stream->stream();
	inStream.read(data.binary(), etMin(pcmBufferSize, pcmDataSize - pcmReadOffset));
	pcmReadOffset += static_cast<size_t>(inStream.gcount());
	
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
	int result = ov_test_callbacks(stream.ptr(), &oggFile, nullptr, -1, oggCallbacks);
	if (result < 0)
	{
		checkOGGError(result, "ov_test_callbacks", _filename.c_str());
		return;
	}
	
	ov_test_open(&oggFile);

	int streamId = -1;
	vorbis_info* info = ov_info(&oggFile, streamId);
	
	bitDepth = 16;
	sampleRate = static_cast<ALsizei>(info->rate);
	channels = info->channels;
	sampleSize = channels * bitDepth / 8;
	
	format = openALFormatFromChannelsAndBitDepth(channels, bitDepth);
	
	size_t oneSecondSize = sampleRate * sampleSize;
	size_t computedDataSize = static_cast<size_t>(sizeof(float) * ov_pcm_total(&oggFile, streamId));
	
	numSamples = computedDataSize / sampleSize;
	pcmDataSize = numSamples * sampleSize;
	pcmBufferSize = BufferDuration * oneSecondSize;
	pcmStartPosition = static_cast<size_t>(ov_pcm_tell(&oggFile));
	oggStartPosition = static_cast<size_t>(ov_raw_tell(&oggFile));
	pcmReadOffset = 0;
	duration = static_cast<float>(pcmDataSize) / static_cast<float>(oneSecondSize);
	totalBuffers = static_cast<int>(1 + pcmDataSize / pcmBufferSize);
	buffersCount = etMin(BuffersCount, totalBuffers);
	alGenBuffers(buffersCount, buffers);
	checkOpenALError("alGenBuffers(%d, ...)", buffersCount);
	
	sourceFormat = SourceFormat_OGG;
}

bool TrackPrivate::fillNextOGGBuffer()
{
	BinaryDataStorage data(pcmBufferSize, 0);
	auto& inStream = stream->stream();
	
	size_t bytesRead = 0;
	while (bytesRead < pcmBufferSize)
	{
		int section = -1;
		long lastRead = ov_read(&oggFile, data.binary() + bytesRead, static_cast<int>(pcmBufferSize - bytesRead), 0, 2, 1, &section);
		
		if (lastRead > 0)
		{
			bytesRead += static_cast<size_t>(lastRead);
		}
		else if (lastRead < 0)
		{
			checkOGGError(lastRead, "ov_read", _filename.c_str());
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
		checkOGGError(result, "ov_raw_seek", _filename.c_str());
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
void checkOGGError(long code, const char* tag, const char* filename)
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
			log::error("Unknown OGG error: %ld at %s\nTrack: %s", code, tag, filename);
			break;
	}
}