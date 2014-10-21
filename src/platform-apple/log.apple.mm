/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Foundation/NSString.h>
#include <Foundation/NSFileHandle.h>

#include <et/core/et.h>
#include <et/platform-apple/apple.h>

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

void ConsoleOutput::debug(const char* fmt, va_list args)
{
#if (ET_DEBUG)
	info(fmt, args);
#else
	(void)fmt;
	(void)args;
#endif
}

void ConsoleOutput::info(const char* fmt, va_list args)
{
#if defined(ET_DENY_NSLOG)
	vprintf(fmt, args);
	printf("\n");
#else
	NSLogv([NSString stringWithFormat:@"%s", fmt], args);
#endif
}

void ConsoleOutput::warning(const char* fmt, va_list args)
{
#if defined(ET_DENY_NSLOG)
	printf("WARNING: ");
	vprintf(fmt, args);
	printf("\n");
#else
	NSLogv([NSString stringWithFormat:@"WARNING: %s", fmt], args);
#endif
}

void ConsoleOutput::error(const char* fmt, va_list args)
{
#if defined(ET_DENY_NSLOG)
	printf("ERROR: ");
	vprintf(fmt, args);
	printf("\n");
#else
	NSLogv([NSString stringWithFormat:@"ERROR: %s", fmt], args);
#endif
}

FileOutput::FileOutput(FILE* file) : _file(file)
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
#else
	(void)format;
	(void)args;
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
