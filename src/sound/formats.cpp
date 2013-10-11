/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et/core/stream.h>
#include <et/sound/openal.h>
#include <et/sound/formats.h>

using namespace et;
using namespace et::audio;

union ChunkIdentifier
{
	char cID[4];
	unsigned char ucID[4];
	uint32_t nID;
};

struct AudioFileChunk
{
	ChunkIdentifier id;
	uint32_t size;
};

const uint32_t AIFFCommonChunkID = 'MMOC';
const uint32_t AIFFUncompressedDataChunkID = 'DNSS';
const uint32_t AIFFCompressedDataChunkID = 'DNSC';

const uint32_t WAVFileChunkID = 'FFIR';
const uint32_t WAVDataChunkID = 'atad';
const uint32_t WAVFormatChunkID = ' tmf';

#pragma pack(1)

struct WAVFileChunk
{
	ChunkIdentifier id;
	uint32_t chunkSize;
	char format[4];
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

struct AIFFFileChunk
{
	ChunkIdentifier ckID;
	uint32_t ckDataSize;
	ChunkIdentifier formType;
};

struct AIFFCommonChunk 
{
	uint16_t numChannels;
	uint32_t numSampleFrames;
	uint16_t sampleSize;

	union
	{
		unsigned char c_sampleRate[10];
		long double sampleRate;
	};

	ChunkIdentifier compressionType;
	char compressionName[256];
};

struct AIFFSoundDataChunk
{
	uint32_t offset;
	uint32_t blockSize;
};

uint32_t swapEndiannes(uint32_t i);
uint16_t swapEndiannes(uint16_t i);
void swapEndiannes(unsigned char* data, size_t dataSize);
double _af_convert_from_ieee_extended (const unsigned char *bytes);

#pragma pack()

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
			assert(false && "Unsupported format in WAV file");
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
			assert(false && "Unsupported format in WAV file");
		}
	}
	else 
	{
		assert(false && "Unsupported number of channels in WAV file");
	}

	return 0;
}

Description::Pointer et::audio::loadWAVFile(const std::string& fileName)
{
	InputStream file(fileName, StreamMode_Binary);
	if (file.invalid())
	{
		std::cout << "Unable to load WAV from file " << fileName << std::endl;
		return Description::Pointer();
	}

	WAVFileChunk fileChunk = { };
	file.stream().read(reinterpret_cast<char*>(&fileChunk), sizeof(fileChunk));
	if (fileChunk.id.nID != WAVFileChunkID)
		return Description::Pointer();

	Description* result = 0;

	while (!file.stream().eof() && file.stream().good())
	{
		AudioFileChunk chunk = { };
		file.stream().read(reinterpret_cast<char*>(&chunk), sizeof(chunk));

		if (chunk.id.nID == WAVFormatChunkID)
		{
			WAVFormatChunk fmt = { };
			file.stream().read(reinterpret_cast<char*>(&fmt), sizeof(fmt));

			result = new Description;
			result->setOrigin(fileName);
			result->sampleRate = fmt.sampleRate;
			result->channels = fmt.numChannels;
			result->bitDepth = fmt.bitsPerSample;
			result->format = openALFormatFromChannelsAndBitDepth(result->channels, result->bitDepth);
		}
		else if ((chunk.id.nID == WAVDataChunkID) && (result != nullptr))
		{
			result->data.resize(chunk.size);
			result->duration = static_cast<float>(result->data.dataSize()) / (
				static_cast<float>(result->sampleRate * result->channels * result->bitDepth / 8));
			file.stream().read(result->data.binary(), chunk.size);
		}
		else if (chunk.size > 0)
		{
			BinaryDataStorage data(static_cast<size_t>(chunk.size));
			file.stream().read(data.binary(), chunk.size);
		}
		else 
		{
			break;
		}
	}

	return Description::Pointer(result);
}

Description::Pointer et::audio::loadCAFFile(const std::string&)
{
	assert(false && "CAF is not currently supported");
	return Description::Pointer();
}

Description::Pointer et::audio::loadAIFFile(const std::string& fileName)
{
	InputStream file(fileName, StreamMode_Binary);
	if (file.invalid())
		return Description::Pointer();

	AIFFFileChunk header = { };
	file.stream().read(reinterpret_cast<char*>(&header), sizeof(header));
	header.ckDataSize = swapEndiannes(header.ckDataSize);

	Description* result = 0;

	while (!file.stream().eof() && !file.stream().fail())
	{
		AudioFileChunk chunk = { };
		file.stream().read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
		chunk.size = swapEndiannes(chunk.size);

		if (chunk.id.nID == AIFFCommonChunkID)
		{
			AIFFCommonChunk comm = { };
			file.stream().read(reinterpret_cast<char*>(&comm), chunk.size);

			result = new Description;
			result->setOrigin(fileName);
			result->bitDepth = swapEndiannes(comm.sampleSize);
			result->channels = swapEndiannes(comm.numChannels);
			result->sampleRate = static_cast<size_t>(_af_convert_from_ieee_extended(comm.c_sampleRate));
			result->format = openALFormatFromChannelsAndBitDepth(result->channels, result->bitDepth);
		}
		else if ((chunk.id.nID == AIFFUncompressedDataChunkID) && (result != nullptr))
		{
			AIFFSoundDataChunk ssnd = { };
			file.stream().read(reinterpret_cast<char*>(&ssnd), sizeof(ssnd));
			file.stream().seekg(ssnd.offset, std::ios::cur);

			result->data.resize(chunk.size - sizeof(ssnd));
			result->duration = static_cast<float>(result->data.dataSize()) / 
				static_cast<float>((result->sampleRate * result->channels * result->bitDepth / 8));
			file.stream().read(result->data.binary(), result->data.dataSize());

			swapEndiannes(result->data.data(), result->data.dataSize());
		}
		else if (chunk.id.nID == AIFFCompressedDataChunkID)
		{
			assert(false && "Compressed audio data is not supported");
		}
		else if (chunk.size > 0)
		{
			BinaryDataStorage data(static_cast<size_t>(chunk.size));
			file.stream().read(data.binary(), chunk.size);
		}
		else 
		{
			break;
		}
	}

	return Description::Pointer(result);
}

Description::Pointer et::audio::loadFile(const std::string& fileName)
{
	size_t dotPos = fileName.rfind('.');
	if (dotPos == std::string::npos)
		return Description::Pointer();

	std::string ext = fileName.substr(dotPos);
	lowercase(ext);

	if (ext == ".wav")
	{
		return loadWAVFile(fileName);
	}
	else if (ext == ".caf")
	{
		return loadCAFFile(fileName);
	}
	else if ((ext == ".aif") || (ext == ".aiff") || (ext == ".aifc"))
	{
		return loadAIFFile(fileName);
	}

	return Description::Pointer();
}

/*
 * Service functions
 */
uint32_t swapEndiannes(uint32_t i)
{
	unsigned char b1 = static_cast<unsigned char>((i & 0x000000ff) >> 0);
	unsigned char b2 = static_cast<unsigned char>((i & 0x0000ff00) >> 8);
	unsigned char b3 = static_cast<unsigned char>((i & 0x00ff0000) >> 16);
	unsigned char b4 = static_cast<unsigned char>((i & 0xff000000) >> 24);
	return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

uint16_t swapEndiannes(uint16_t i)
{
	unsigned char b1 = static_cast<unsigned char>((i & 0x00ff) >> 0);
	unsigned char b2 = static_cast<unsigned char>((i & 0xff00) >> 8);
	return static_cast<uint16_t>((b1 << 16) | b2);
}

void swapEndiannes(unsigned char* data, size_t dataSize)
{
	uint32_t* ptr = reinterpret_cast<uint32_t*>(data);
	for (size_t i = 0; i < dataSize / 4; ++i)
    {
        uint32_t val = *ptr;
		*ptr++ = swapEndiannes(val);
    }
}

#define UnsignedToFloat(u) (((double) ((long) (u - 2147483647L - 1))) + 2147483648.0)
double _af_convert_from_ieee_extended (const unsigned char *bytes)
{
	double			f;
	int				expon;
	unsigned long	hiMant, loMant;
	
	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	
	hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24)
		| ((unsigned long) (bytes[3] & 0xFF) << 16)
		| ((unsigned long) (bytes[4] & 0xFF) << 8)
		| ((unsigned long) (bytes[5] & 0xFF));
	
	loMant = ((unsigned long) (bytes[6] & 0xFF) << 24)
		| ((unsigned long) (bytes[7] & 0xFF) << 16)
		| ((unsigned long) (bytes[8] & 0xFF) << 8)
		| ((unsigned long) (bytes[9] & 0xFF));
	
	if ((expon == 0) && (hiMant == 0) && (loMant == 0))
	{
		f = 0;
	}
	else
	{
		if (expon == 0x7FFF) /* Infinity or NaN */
		{
			f = HUGE_VAL;
		}
		else
		{
			expon -= 16383;
			f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
			f += ldexp(UnsignedToFloat(loMant), expon-=32);
		}
	}
	
	return (bytes[0] & 0x80) ? -f : f;
}
