/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <fstream>
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
		InputStreamPrivate() : stream(0)
			{ }
		
		~InputStreamPrivate()
			{ delete stream; }

	public:
		std::istream* stream;
	};
}

using namespace et;

InputStream::InputStream() : _private(new InputStreamPrivate)
{
}

InputStream::InputStream(const std::string& file, StreamMode mode) :
	_private(new InputStreamPrivate)
{
	std::ios::openmode openMode = std::ios::in;
	
	if (mode == StreamMode_Binary)
		openMode |= std::ios::binary;
	
	_private->stream = new std::ifstream(file.c_str(), openMode);

	if (_private->stream->fail())
	{
		log::error("Unable to load file: %s", file.c_str());
		delete _private->stream;
		_private->stream = nullptr;
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
