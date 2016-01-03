/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

#define PASS_TO_OUTPUTS(FUNC)		for (Output::Pointer output : logOutputs) \
									{ \
										va_list args; \
										va_start(args, format); \
										output->FUNC(format, args); \
										va_end(args); \
									}

using namespace et;
using namespace log;

static std::vector<Output::Pointer> logOutputs;

void et::log::addOutput(Output::Pointer ptr)
{
	logOutputs.push_back(ptr);
}

void et::log::removeOutput(Output::Pointer ptr)
{
	logOutputs.erase(std::remove_if(logOutputs.begin(), logOutputs.end(),
		[ptr](Output::Pointer out) { return out == ptr; }), logOutputs.end());
}

void et::log::debug(const char* format, ...) { PASS_TO_OUTPUTS(debug) }
void et::log::info(const char* format, ...) { PASS_TO_OUTPUTS(info) }
void et::log::warning(const char* format, ...) { PASS_TO_OUTPUTS(warning) }
void et::log::error(const char* format, ...) { PASS_TO_OUTPUTS(error) }

ConsoleOutput::ConsoleOutput() :
	FileOutput(stdout)
{
}

void ConsoleOutput::debug(const char* format, va_list args)
{
#if (ET_DEBUG)
	info(format, args);
#endif
}

void ConsoleOutput::info(const char* format, va_list args)
{
	static char storage[2048] = { };
	int pos = vsnprintf(storage, sizeof(storage), format, args);
	pos = etMin(pos, static_cast<int>(sizeof(storage) - 3));
	storage[pos++] = '\n';
	storage[pos++] = 0;
	OutputDebugStringA(storage);
	printf(storage);
}

void ConsoleOutput::warning(const char* fmt, va_list args)
{
	OutputDebugStringA("WARNING: ");
	info(fmt, args);
}

void ConsoleOutput::error(const char* fmt, va_list args)
{
	OutputDebugStringA("ERROR: ");
	info(fmt, args);
}

FileOutput::FileOutput(FILE* file) :
	_file(file)
{
	if (file == nullptr)
	{
		_file = stdout;
		fprintf(_file, "Invalid file was provided to FileOutput, output will be redirected to console.");
	}
}

FileOutput::FileOutput(const std::string& filename)
{
	_file = fopen(filename.c_str(), "w");
	if (_file == nullptr)
	{
		printf("Unable to open %s for writing, output will be redirected to console.", filename.c_str());
		_file = stdout;
	}
}

FileOutput::~FileOutput()
{
	if ((_file != nullptr) && (_file != stdout))
	{
		fflush(_file);
		fclose(_file);
	}
}

void FileOutput::debug(const char* format, va_list args)
{
#if (ET_DEBUG)
	info(format, args);
#endif
}

void FileOutput::info(const char* format, va_list args)
{
	vfprintf(_file, format, args);
	fprintf(_file, "\n");
	fflush(_file);
}

void FileOutput::warning(const char* format, va_list args)
{
	fprintf(_file, "WARNING: ");
	info(format, args);
}

void FileOutput::error(const char* format, va_list args)
{
	fprintf(_file, "ERROR: ");
	info(format, args);
}

#endif // ET_PLATFORM_WIN
