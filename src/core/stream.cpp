/*
* This file is part of `et engine`
* Copyright 2009-2014 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/core/et.h>

#if (ET_PLATFORM_ANDROID)
#
#	error Please include platform-android implementation instead
#
#endif

namespace et
{
	class InputStreamPrivate
	{
	public:
		~InputStreamPrivate()
		{
			sharedObjectFactory().deleteObject(stream);
		}

	public:
		std::istream* stream = nullptr;
	};
}

using namespace et;

InputStream::InputStream()
{
	ET_PIMPL_INIT(InputStream)
}

InputStream::InputStream(const std::string& file, StreamMode mode)
{
	ET_PIMPL_INIT(InputStream)
	
	std::ios::openmode openMode = std::ios::in;
	
	if (mode == StreamMode_Binary)
		openMode |= std::ios::binary;
	
	_private->stream = sharedObjectFactory().createObject<std::ifstream>(file.c_str(), openMode);

	if (_private->stream->fail())
	{
		log::error("Unable to open file: %s", file.c_str());
		sharedObjectFactory().deleteObject(_private->stream);
		_private->stream = nullptr;
	}
}

InputStream::~InputStream()
{
	ET_PIMPL_FINALIZE(InputStream)
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
