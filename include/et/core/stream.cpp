/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

namespace et
{
class InputStreamPrivate
{
public:
	~InputStreamPrivate()
	{
		etDestroyObject(stream);
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
	
	_private->stream = etCreateObject<std::ifstream>(file.c_str(), openMode);

	if (_private->stream->fail())
	{
		log::error("Unable to open file: %s", file.c_str());
		etDestroyObject(_private->stream);
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
