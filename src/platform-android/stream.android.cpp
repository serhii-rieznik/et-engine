/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <libzip/zip.h>

#include <et/core/et.h>
#include <et/core/stream.h>
#include <et/core/containers.h>

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

InputStream::InputStream() : _private(new InputStreamPrivate)
{
}

InputStream::InputStream(const std::string& file, StreamMode mode) : _private(new InputStreamPrivate)
{
	std::ios::openmode openMode = std::ios::in;
	if (mode == StreamMode_Binary)
		openMode |= std::ios::binary;
	
	zip* a = sharedAndroidZipArchive();

	_private->zipFileIndex = zip_name_locate(a, file.c_str(), 0);
	if (_private->zipFileIndex == -1)
	{
		if (access(file.c_str(), 0) == 0)
		{
			struct stat status = { };
			stat(file.c_str(), &status);
			if ((status.st_mode & S_IFREG) == S_IFREG)
			{
				_private->stream = new std::ifstream(file.c_str(), openMode);
			}
			else
			{
				log::error("%s is not a file.", file.c_str());
			}
		}
		else
		{
			log::error("Unable to open file %s", file.c_str());
		}
	}
	else
	{
		zip_error_clear(a);
		_private->zipFile = zip_fopen_index(a, _private->zipFileIndex, 0);
		
		if (_private->zipFile == nullptr)
		{
			log::error("Unable to open file %s at index %d. Error: %s",
				file.c_str(), _private->zipFileIndex, zip_strerror(a));
			return;
		}
		
		struct zip_stat stat;
		zip_stat_init(&stat);
		
		zip_error_clear(a);
		int result = zip_stat_index(a, _private->zipFileIndex, 0, &stat);
		if (stat.size == 0)
		{
			log::error("Unable to get file %s stats.", file.c_str());
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
