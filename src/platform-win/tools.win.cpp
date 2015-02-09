/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>

#if (ET_PLATFORM_WIN)

#include <et/platform/platformtools.h>
#include <et/core/filesystem.h>
#include <et/core/containers.h>

#include <Shlobj.h>
#include <ShellAPI.h>
#include <CommDlg.h>

#if defined(_UNICODE)
#	define ET_WIN_UNICODE					1
#	define ET_CHAR_TYPE						wchar_t
#	define ET_STRING_TYPE					std::wstring
#	define ET_STRING_TO_PARAM_TYPE(str)		utf8ToUnicode(str)
#	define ET_STRING_TO_OUTPUT_TYPE(str)	unicodeToUtf8(str)
#	define ET_STRING_FROM_CONST_CHAR(str)	L##str
#else
#	define ET_WIN_UNICODE					0
#	define ET_CHAR_TYPE						char
#	define ET_STRING_TYPE					std::string
#	define ET_STRING_TO_PARAM_TYPE(str)		str
#	define ET_STRING_TO_OUTPUT_TYPE(str)	str
#	define ET_STRING_FROM_CONST_CHAR(str)	str
#endif

static bool shouldInitializeTime = true;

static uint64_t performanceFrequency = 0;
static uint64_t initialCounter = 0;

static const ET_STRING_TYPE currentFolder = ET_STRING_FROM_CONST_CHAR(".");
static const ET_STRING_TYPE previousFolder = ET_STRING_FROM_CONST_CHAR("..");
static const ET_STRING_TYPE allFilesMask = ET_STRING_FROM_CONST_CHAR("*.*");

void initTime()
{
	LARGE_INTEGER c = { };
	LARGE_INTEGER f = { };

	shouldInitializeTime = false;

	QueryPerformanceCounter(&c);
	QueryPerformanceFrequency(&f);

	initialCounter = c.QuadPart;
	performanceFrequency = f.QuadPart;
}

uint64_t et::queryContiniousTimeInMilliSeconds()
{
	if (shouldInitializeTime)
		initTime();

	LARGE_INTEGER c = { };
	QueryPerformanceCounter(&c);

	return 1000 * (c.QuadPart - initialCounter) / performanceFrequency;
}

float et::queryContiniousTimeInSeconds()
{
	return static_cast<float>(queryContiniousTimeInMilliSeconds()) / 1000.0f;
}

uint64_t et::queryCurrentTimeInMicroSeconds()
{
	LARGE_INTEGER c = { };
	QueryPerformanceCounter(&c);
	return 1000000 * c.QuadPart / performanceFrequency;
}

const char et::pathDelimiter = '\\';
const char et::invalidPathDelimiter = '/';

std::string et::applicationPath()
{
	char ExePath[MAX_PATH] = { };
	GetModuleFileNameA(nullptr, ExePath, MAX_PATH);
	return getFilePath(normalizeFilePath(ExePath));
}

bool et::fileExists(const std::string& name)
{
	auto attr = GetFileAttributes(ET_STRING_TO_PARAM_TYPE(name).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
}

bool et::folderExists(const std::string& folder)
{
	auto attr = GetFileAttributes(ET_STRING_TO_PARAM_TYPE(folder).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

void et::findFiles(const std::string& folder, const std::string& mask, bool recursive, StringList& list)
{
	ET_STRING_TYPE normalizedFolder = ET_STRING_TO_PARAM_TYPE(addTrailingSlash(folder));
	ET_STRING_TYPE searchPath = normalizedFolder + ET_STRING_TO_PARAM_TYPE(mask);

	StringList folderList;
	if (recursive)
	{
		ET_STRING_TYPE foldersSearchPath = normalizedFolder + allFilesMask;

		WIN32_FIND_DATA folders = { };
		HANDLE folderSearch = FindFirstFile(foldersSearchPath.c_str(), &folders);
		if (folderSearch != INVALID_HANDLE_VALUE)
		{
			do
			{
				bool isFolder = (folders.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
				ET_STRING_TYPE name(folders.cFileName);
				if (isFolder && (name != currentFolder) && (name != previousFolder))
					folderList.push_back(ET_STRING_TO_OUTPUT_TYPE(normalizedFolder + name));

			}
			while (FindNextFile(folderSearch, &folders));
			FindClose(folderSearch);
		}
	}

	WIN32_FIND_DATA data = { };
	HANDLE search = FindFirstFile(searchPath.c_str(), &data);
	if (search != INVALID_HANDLE_VALUE)
	{
		do
		{
			bool isAcceptable = 
				((data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) == FILE_ATTRIBUTE_NORMAL) || 
				((data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) == FILE_ATTRIBUTE_ARCHIVE) ||
				((data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY);

			ET_STRING_TYPE name(data.cFileName);

			if (isAcceptable && (name != currentFolder) && (name != previousFolder))
				list.push_back(ET_STRING_TO_OUTPUT_TYPE(normalizedFolder + name));
		}
		while (FindNextFile(search, &data));
		FindClose(search);
	}

	if (recursive)
	{
		for (const std::string& i : folderList)
			findFiles(i, mask, recursive, list);
	}
}

std::string et::applicationPackagePath()
{
	ET_CHAR_TYPE buffer[MAX_PATH] = { };
	GetCurrentDirectory(MAX_PATH, buffer);
	return addTrailingSlash(ET_STRING_TO_OUTPUT_TYPE(buffer));
}

std::string et::applicationDataFolder()
{
	ET_CHAR_TYPE buffer[MAX_PATH] = { };
	GetCurrentDirectory(MAX_PATH, buffer);
	return addTrailingSlash(ET_STRING_TO_OUTPUT_TYPE(buffer));
}

std::string et::documentsBaseFolder()
{
	wchar_t* path = nullptr;
	SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

	if (path == nullptr) 
		return emptyString;

	std::string result = addTrailingSlash(unicodeToUtf8(path));
	CoTaskMemFree(path);
	return result;
}

std::string et::libraryBaseFolder()
{
	wchar_t* path = nullptr;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

	if (path == nullptr) 
		return emptyString;

	std::string result = addTrailingSlash(unicodeToUtf8(path));
	CoTaskMemFree(path);
	return result;
}

bool et::createDirectory(const std::string& name, bool recursive)
{
	if (recursive)
	{
		char delim[] = {pathDelimiter, 0};
		bool gotError = false;
		StringList components = split(name, std::string(delim));
		std::string path;
		for (auto& dir : components)
		{
			path += dir + "\\";
			if (!folderExists(path))
				gotError |= ::CreateDirectory(ET_STRING_TO_PARAM_TYPE(path).c_str(), nullptr) == 0;
		}

		return !gotError;
	}

	return ::CreateDirectory(ET_STRING_TO_PARAM_TYPE(name).c_str(), nullptr) == 0;
}

bool et::removeFile(const std::string& name)
{
	ET_STRING_TYPE aName = ET_STRING_TO_PARAM_TYPE(name);
	aName.resize(aName.size() + 1);
	aName.back() = 0;

	SHFILEOPSTRUCT fop = { };

	fop.wFunc = FO_DELETE;
	fop.pFrom = aName.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperation(&fop) == 0;
}

bool et::copyFile(const std::string& from, const std::string& to)
{
	ET_STRING_TYPE aFrom= ET_STRING_TO_PARAM_TYPE(from);
	ET_STRING_TYPE aTo = ET_STRING_TO_PARAM_TYPE(to);
	aFrom.resize(aFrom.size() + 1);
	aTo.resize(aTo.size() + 1);
	aFrom.back() = 0;
	aTo.back() = 0;

	SHFILEOPSTRUCT fop = { };

	fop.wFunc = FO_COPY;
	fop.pFrom = aFrom.c_str();
	fop.pTo = aTo.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperation(&fop) == 0;
}

bool et::removeDirectory(const std::string& name)
{
	ET_STRING_TYPE aName = ET_STRING_TO_PARAM_TYPE(name);
	aName.resize(aName.size() + 1);
	aName.back() = 0;

	SHFILEOPSTRUCT fop = { };

	fop.wFunc = FO_DELETE;
	fop.pFrom = aName.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperation(&fop) == 0;
}

void et::findSubfolders(const std::string& folder, bool recursive, StringList& list)
{
	StringList folderList;
	
	ET_STRING_TYPE normalizedFolder = ET_STRING_TO_PARAM_TYPE(addTrailingSlash(folder));
	ET_STRING_TYPE foldersSearchPath = normalizedFolder + allFilesMask;

	WIN32_FIND_DATA folders = { };
	HANDLE folderSearch = FindFirstFile(foldersSearchPath.c_str(), &folders);
	if (folderSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			ET_STRING_TYPE name(folders.cFileName);
			bool isFolder = (folders.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
			if (isFolder && (name != currentFolder) && (name != previousFolder))
			{
				folderList.push_back(ET_STRING_TO_OUTPUT_TYPE(normalizedFolder + 
					name + ET_STRING_FROM_CONST_CHAR("\\")));
			}
		}
		while (FindNextFile(folderSearch, &folders));
		FindClose(folderSearch);
	}

	if (recursive)
	{
		for (const std::string& i : folderList)
			findSubfolders(i, true, list);
	}

	list.insert(list.end(), folderList.begin(), folderList.end());
}

void et::openUrl(const std::string& url)
{
	ShellExecute(nullptr, ET_STRING_FROM_CONST_CHAR("open"), 
		ET_STRING_TO_PARAM_TYPE(url).c_str(), 0, 0, SW_SHOWNORMAL);
}

std::string et::unicodeToUtf8(const std::wstring& w)
{
	int mbcWidth = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, 0, 0, 0, 0);

	if (mbcWidth == 0)
		return emptyString;

	DataStorage<char> result(mbcWidth + 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, result.data(), static_cast<int>(result.size()), 0, 0);

	return std::string(result.data());
}

std::wstring et::utf8ToUnicode(const std::string& mbcs)
{
	int uWidth = MultiByteToWideChar(CP_UTF8, 0, mbcs.c_str(), -1, 0, 0);
	if (uWidth == 0)
	{
		switch (GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
			std::cout << "A supplied buffer size was not large enough, or it was incorrectly set to NULL." << std::endl;
			break;

		case ERROR_INVALID_FLAGS:
			std::cout << "The values supplied for flags were not valid." << std::endl;
			break;

		case ERROR_INVALID_PARAMETER:
			std::cout << "Any of the parameter values was invalid." << std::endl;
			break;

		case ERROR_NO_UNICODE_TRANSLATION:
			std::cout << "Invalid Unicode was found in a string" << std::endl;
			break;

		default:
			std::cout << "Failed to convert utf-8 to wchar_t" << std::endl;
		}

		return std::wstring();
	}

	DataStorage<wchar_t> result(uWidth + 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, mbcs.c_str(), -1, result.data(), static_cast<int>(result.size()));

	return std::wstring(result.data());
}

std::string et::applicationIdentifierForCurrentProject()
	{ return "com.et.app"; }

uint64_t et::getFileDate(const std::string& path)
{
	WIN32_FIND_DATA findData = { };
	HANDLE search = FindFirstFile(ET_STRING_TO_PARAM_TYPE(path).c_str(), &findData);
	FindClose(search);

	return findData.ftLastWriteTime.dwLowDateTime |
		(static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32);
}

std::vector<et::Screen> et::availableScreens()
{
	std::vector<et::Screen> result;
	
	EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) -> BOOL
	{
		MONITORINFO info = { sizeof(MONITORINFO) };

		GetMonitorInfo(hMonitor,  &info);

		recti screenRect(info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
			info.rcMonitor.bottom - info.rcMonitor.top);

		recti workarea(info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left,
			info.rcWork.bottom - info.rcWork.top);

		std::vector<et::Screen>* r = reinterpret_cast<std::vector<et::Screen>*>(dwData);
		r->emplace_back(screenRect, workarea, 1);
		return 1;
	}, 
	reinterpret_cast<LPARAM>(&result));

	return result;
}

std::string et::selectFile(const StringList&, SelectFileMode mode, const std::string& defaultName)
{
	ET_STRING_TYPE defaultFileName = ET_STRING_TO_PARAM_TYPE(defaultName);

	DataStorage<ET_CHAR_TYPE> defaultFileNameData(etMax(size_t(MAX_PATH), defaultFileName.size()) + 1, 0);
	etCopyMemory(defaultFileNameData.data(), defaultFileName.data(), defaultFileName.size());

	OPENFILENAME of = { };
	of.lStructSize = sizeof(of);
	of.hInstance = GetModuleHandle(nullptr);
	of.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
	of.lpstrFile = defaultFileNameData.data();
	of.nMaxFile = MAX_PATH;

#if (_UNICODE)
	of.lpstrFilter = L"All Files\0*.*\0\0";
#else
	of.lpstrFilter = "All Files\0*.*\0\0";
#endif

	auto func = (mode == SelectFileMode::Save) ? GetSaveFileName : GetOpenFileName;

	return func(&of) ? ET_STRING_TO_OUTPUT_TYPE(of.lpstrFile) : emptyString;
}

#endif // ET_PLATFORM_WIN
