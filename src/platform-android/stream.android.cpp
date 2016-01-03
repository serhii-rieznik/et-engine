/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/et.h>
#include <et/core/stream.h>
#include <et/core/containers.h>

#if (ET_PLATFORM_ANDROID)

#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <libzip/zip.h>

namespace et
{
	extern zip* sharedAndroidZipArchive();

	class InputStreamPrivate
	{
	public:
		InputStreamPrivate() :
			zipFileIndex(0), zipFile(0), stream(0) { }

		~InputStreamPrivate()
		{
			if (zipFile)
				zip_fclose(zipFile);
			
			delete stream;
		}

	public:
		int zipFileIndex;
		zip_file* zipFile;

		std::istream* stream;
	};
}

using namespace et;

InputStream::InputStream() :
	_private(new InputStreamPrivate)
{
}

InputStream::InputStream(const std::string& inFile, StreamMode mode) :
	_private(new InputStreamPrivate)
{
	std::string normalizedPath;
	do
	{
		normalizedPath = removeUpDir(inFile);
	} while (normalizedPath.find("..") != std::string::npos);
	
	std::ios::openmode openMode = std::ios::in;
	if (mode == StreamMode_Binary)
		openMode |= std::ios::binary;
	
	zip* a = sharedAndroidZipArchive();

	_private->zipFileIndex = zip_name_locate(a, normalizedPath.c_str(), 0);
	if (_private->zipFileIndex == -1)
	{
		if (access(normalizedPath.c_str(), 0) == 0)
		{
			struct stat status = { };
			stat(normalizedPath.c_str(), &status);
			if ((status.st_mode & S_IFREG) == S_IFREG)
			{
				_private->stream = new std::ifstream(normalizedPath.c_str(), openMode);
			}
			else
			{
				log::error("%s is not a file.", normalizedPath.c_str());
			}
		}
		else
		{
			log::error("Unable to open file %s", normalizedPath.c_str());
		}
	}
	else
	{
		zip_error_clear(a);
		_private->zipFile = zip_fopen_index(a, _private->zipFileIndex, 0);
		
		if (_private->zipFile == nullptr)
		{
			log::error("Unable to open file %s at index %d. Error: %s",
				normalizedPath.c_str(), _private->zipFileIndex, zip_strerror(a));
			return;
		}
		
		struct zip_stat stat;
		zip_stat_init(&stat);
		
		zip_error_clear(a);
		int result = zip_stat_index(a, _private->zipFileIndex, 0, &stat);
		if (stat.size == 0)
		{
			log::error("Unable to get file %s stats.", normalizedPath.c_str());
		}
		else
		{
			std::string data(stat.size + 1, 0);
			zip_fread(_private->zipFile, &data[0], stat.size);
			_private->stream = new std::istringstream(data, openMode);
		}
	}
}

InputStream::~InputStream()
{
	delete _private;
}

bool InputStream::valid()
{
	return (_private->stream != nullptr) && (_private->stream->good());
}

bool InputStream::invalid()
{
	return (_private->stream == nullptr);
}

std::istream& InputStream::stream()
{
	if (invalid())
		log::error("Accessing invalid stream.");
	
	return *_private->stream;
}

#endif // ET_PLATFORM_ANDROID
