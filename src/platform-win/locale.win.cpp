/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <fstream>
#include <Windows.h>
#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.h>

using namespace et;

std::string Locale::time()
{
	SYSTEMTIME st = { };
	GetSystemTime(&st);

	int bufferSize = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, 0, 0);
	DataStorage<wchar_t> buffer(bufferSize + 1, 0);
	GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, &st, 0, buffer.data(), buffer.size());
	return unicodeToUtf8(buffer.data());
}

std::string Locale::date()
{
	SYSTEMTIME st = { };
	GetSystemTime(&st);

	int bufferSize = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, 0, 0, 0, 0);
	DataStorage<wchar_t> buffer(bufferSize + 1, 0);
	GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_LONGDATE, &st, 0, buffer.data(), buffer.size(), 0);
	return unicodeToUtf8(buffer.data());
}

size_t Locale::currentLocale()
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

		return 0;
	}

	std::string mbcs = unicodeToUtf8(localeData);

	size_t result = 0;

	if ((mbcs.size() == 5) && (mbcs[2] == '-'))
	{
		lowercase(mbcs);
		result = mbcs[0] | (mbcs[1] << 8) | (mbcs[3] << 16) | (mbcs[4] << 24);
	}

	return result;
}
