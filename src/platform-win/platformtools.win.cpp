/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform/platformtools.h>

#if (ET_PLATFORM_WIN)

#include <CommDlg.h>

using namespace et;

std::string et::selectFile(const StringList&, SelectFileMode mode, const std::string& defaultName)
{
#	pragma message("DEAL WITH UNICODE HERE")

	char filename[1024] = { };
	etCopyMemory(filename, defaultName.c_str(), defaultName.length());

	OPENFILENAMEA of = { };
	of.lStructSize = sizeof(of);
	of.hwndOwner = 0;
	of.hInstance = GetModuleHandle(0);
	of.lpstrFilter = "All files\0*.*\0\0";
	of.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
	of.lpstrFile = filename;
	of.nMaxFile = MAX_PATH;

	if (mode == SelectFileMode_Open)
		return GetOpenFileNameA(&of) ? std::string(filename) : emptyString;
	else 
		return GetSaveFileNameA(&of) ? std::string(filename) : emptyString;
}

#endif // ET_PLATFORM_WIN
