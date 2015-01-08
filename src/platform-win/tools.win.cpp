/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/core/filesystem.h>
#include <et/core/containers.h>

#if (ET_PLATFORM_WIN)

#include <Shlobj.h>
#include <ShellAPI.h>

static bool shouldInitializeTime = true;
static uint64_t performanceFrequency = 0;
static uint64_t initialCounter = 0;

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
	auto attr = GetFileAttributesW(utf8ToUnicode(name).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
}

bool et::folderExists(const std::string& folder)
{
	auto attr = GetFileAttributesW(utf8ToUnicode(folder).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

void et::findFiles(const std::string& folder, const std::string& mask, bool recursive, StringList& list)
{
	std::wstring normalizedFolder = utf8ToUnicode(addTrailingSlash(folder));
	std::wstring searchPath = normalizedFolder + utf8ToUnicode(mask);

	StringList folderList;
	if (recursive)
	{
		std::wstring foldersSearchPath = normalizedFolder + L"*.*";
		WIN32_FIND_DATAW folders = { };
		HANDLE folderSearch = FindFirstFileW(foldersSearchPath.c_str(), &folders);
		if (folderSearch != INVALID_HANDLE_VALUE)
		{
			do
			{
				bool isFolder = (folders.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
				std::wstring name(folders.cFileName);
				if (isFolder && (name !=  L".") && (name != L".."))
					folderList.push_back(unicodeToUtf8(normalizedFolder + name));
			}
			while (FindNextFile(folderSearch, &folders));
			FindClose(folderSearch);
		}
	}

	WIN32_FIND_DATA data = { };
	HANDLE search = FindFirstFileW(searchPath.c_str(), &data);
	if (search != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring name(data.cFileName);
			if ((name != L".") && (name != L".."))
				list.push_back(unicodeToUtf8(normalizedFolder + name));
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
	wchar_t buffer[MAX_PATH] = { };
	GetCurrentDirectoryW(MAX_PATH, buffer);
	return addTrailingSlash(unicodeToUtf8(buffer));
}

std::string et::applicationDataFolder()
{
	wchar_t buffer[MAX_PATH] = { };
	GetCurrentDirectoryW(MAX_PATH, buffer);
	return addTrailingSlash(unicodeToUtf8(buffer));
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
			{
				std::wstring wPath = utf8ToUnicode(path);
				gotError |= ::CreateDirectoryW(wPath.c_str(), nullptr) == 0;
			}
		}

		return !gotError;
	}
	else
	{
		return ::CreateDirectoryW(utf8ToUnicode(name).c_str(), nullptr) == 0;
	}
}

bool et::removeFile(const std::string& name)
{
#	pragma message("DEAL WITH UNICODE HERE")

	std::string doubleNullTerminated(name.size() + 1, 0);
	std::copy(name.begin(), name.end(), doubleNullTerminated.begin());

	SHFILEOPSTRUCTA fop = {};

	fop.hwnd = 0;
	fop.wFunc = FO_DELETE;
	fop.pFrom = doubleNullTerminated.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperationA(&fop) == 0;
}

bool et::copyFile(const std::string& from, const std::string& to)
{
#	pragma message("DEAL WITH UNICODE HERE")

	std::string fromNullTerminated(from.size() + 1, 0);
	std::copy(from.begin(), from.end(), fromNullTerminated.begin());

	std::string toNullTerminated(to.size() + 1, 0);
	std::copy(to.begin(), to.end(), toNullTerminated.begin());

	SHFILEOPSTRUCTA fop = {};

	fop.hwnd = 0;
	fop.wFunc = FO_COPY;
	fop.pFrom = fromNullTerminated.c_str();
	fop.pTo = toNullTerminated.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperationA(&fop) == 0;
}

bool et::removeDirectory(const std::string& name)
{
#	pragma message("DEAL WITH UNICODE HERE")

	std::string doubleNullTerminated(name.size() + 1, 0);
	std::copy(name.begin(), name.end(), doubleNullTerminated.begin());
	
	SHFILEOPSTRUCTA fop = { };

	fop.hwnd = 0;
	fop.wFunc = FO_DELETE;
	fop.pFrom = doubleNullTerminated.c_str();
	fop.fFlags = FOF_NO_UI;

	return SHFileOperationA(&fop) == 0;
}

void et::findSubfolders(const std::string& folder, bool recursive, StringList& list)
{
	std::wstring normalizedFolder = utf8ToUnicode(addTrailingSlash(folder));
	std::wstring foldersSearchPath = normalizedFolder + L"*.*";
	StringList folderList;

	WIN32_FIND_DATAW folders = { };
	HANDLE folderSearch = FindFirstFileW(foldersSearchPath.c_str(), &folders);
	if (folderSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wstring name(folders.cFileName);
			bool isFolder = (folders.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
			if (isFolder && (name != L".") && (name != L".."))
				folderList.push_back(unicodeToUtf8(normalizedFolder + name + L"\\"));
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
	ShellExecuteW(0, L"open", utf8ToUnicode(url).c_str(), 0, 0, SW_SHOWNORMAL);
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
	HANDLE search = FindFirstFile(utf8ToUnicode(path).c_str(), &findData);
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

#endif // ET_PLATFORM_WIN
