/*
* This file is part of `et engine`
* Copyright 2009-2014 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.h>

#if (ET_PLATFORM_WIN)

#include <fstream>
#include <Windows.h>

using namespace et;

std::string locale::time()
{
	SYSTEMTIME st = { };
	GetSystemTime(&st);

	int bufferSize = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, 0, 0);
	DataStorage<wchar_t> buffer(bufferSize + 1, 0);
	GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, buffer.data(), buffer.size());
	return unicodeToUtf8(buffer.data());
}

std::string locale::date()
{
	SYSTEMTIME st = { };
	GetSystemTime(&st);

	int bufferSize = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, 0, 0, 0, 0);
	DataStorage<wchar_t> buffer(bufferSize + 1, 0);
	GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, 0, buffer.data(), buffer.size(), 0);
	return unicodeToUtf8(buffer.data());
}

std::string locale::dateTimeFromTimestamp(uint64_t t)
{
	FILETIME ft = {};

	auto ll = Int32x32To64(t, 10000000) + 116444736000000000;
	ft.dwLowDateTime = static_cast<DWORD>(ll);
	ft.dwHighDateTime = static_cast<DWORD>(ll >> 32);

	SYSTEMTIME st = { };
	FileTimeToSystemTime(&ft, &st);

	int dateBufferSize = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, 0, 0, 0, 0);
	int timeBufferSize = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, 0, 0);

	DataStorage<wchar_t> dateBuffer(dateBufferSize + 1, 0);
	GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_SHORTDATE, &st, 0, dateBuffer.data(), dateBuffer.size(), 0);

	DataStorage<wchar_t> timeBuffer(dateBufferSize + 1, 0);
	GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, timeBuffer.data(), timeBuffer.size());

	return unicodeToUtf8(dateBuffer.data()) + " " + unicodeToUtf8(timeBuffer.data());
}

std::string locale::currentLocale()
{
	wchar_t localeName[256] = { };
	wchar_t localeData[256] = { };

	GetUserDefaultLocaleName(localeName, 256);
	if (GetLocaleInfoEx(localeName, LOCALE_SNAME, localeData, 256) == 0)
	{
		switch (GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
			{
				std::cout << "A supplied buffer size was not large enough, or it was incorrectly set to NULL." << std::endl;
				break;
			}
		case ERROR_INVALID_FLAGS:
			{
				std::cout << "The values supplied for flags were not valid." << std::endl;
				break;
			}
		case ERROR_INVALID_PARAMETER:
			{
				std::cout << "Any of the parameter values was invalid." << std::endl;
			}

		default:
			std::cout << "Unknown GetLocaleInfoEx error" << std::endl;
		}

		return std::string("en");
	}

	return lowercase(unicodeToUtf8(localeData));
}

#endif // ET_PLATFORM_WIN
