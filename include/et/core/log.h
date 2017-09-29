/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#if defined (ET_CORE_INCLUDES)

namespace et
{
namespace log
{
struct Output : public Object
{
	ET_DECLARE_POINTER(Output);

	virtual void debug(const char*, va_list) {}
	virtual void info(const char*, va_list) {}
	virtual void warning(const char*, va_list) {}
	virtual void error(const char*, va_list) {}
};

void addOutput(Output::Pointer);
void removeOutput(Output::Pointer);

class FileOutput : public Output
{
public:
	ET_DECLARE_POINTER(FileOutput);

public:
	FileOutput(const std::string&);
	FileOutput(FILE*);

	~FileOutput();

	void debug(const char*, va_list);
	void info(const char*, va_list);
	void warning(const char*, va_list);
	void error(const char*, va_list);

private:
	FILE* _file = nullptr;
};

class ConsoleOutput : public FileOutput
{
public:
	ET_DECLARE_POINTER(ConsoleOutput);

	void debug(const char*, va_list);
	void info(const char*, va_list);
	void warning(const char*, va_list);
	void error(const char*, va_list);

	void info(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		info(format, args);
		va_end(args);
	}

public:
	ConsoleOutput();
};
}
}

#endif
