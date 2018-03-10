/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>

#if (ET_PLATFORM_WIN)

#include <et/core/hardware.h>
#include <et/core/filesystem.h>
#include <et/core/containers.h>
#include <et/app/application.h>

#include <Shlobj.h>
#include <ShellAPI.h>
#include <CommDlg.h>

static uint64_t performanceFrequency = 0;
static uint64_t initialCounter = 0;

static const ET_STRING_TYPE currentFolder = ET_STRING_FROM_CONST_CHAR(".");
static const ET_STRING_TYPE previousFolder = ET_STRING_FROM_CONST_CHAR("..");
static const ET_STRING_TYPE allFilesMask = ET_STRING_FROM_CONST_CHAR("*.*");

namespace et
{

void initTime()
{
	LARGE_INTEGER c = { };
	LARGE_INTEGER f = { };

	QueryPerformanceCounter(&c);
	QueryPerformanceFrequency(&f);

	initialCounter = c.QuadPart;
	performanceFrequency = f.QuadPart;
}

uint64_t queryContinuousTimeInMilliSeconds()
{
	if (performanceFrequency == 0)
		initTime();

	LARGE_INTEGER c = { };
	QueryPerformanceCounter(&c);

	return 1000 * (c.QuadPart - initialCounter) / performanceFrequency;
}

float queryContinuousTimeInSeconds()
{
	return static_cast<float>(queryContinuousTimeInMilliSeconds()) / 1000.0f;
}

uint64_t queryCurrentTimeInMicroSeconds()
{
	if (performanceFrequency == 0)
		initTime();

	LARGE_INTEGER c = { };
	QueryPerformanceCounter(&c);
	return 1000000 * c.QuadPart / performanceFrequency;
}

const char pathDelimiter = '\\';
const char invalidPathDelimiter = '/';

std::string applicationPath()
{
	ET_CHAR_TYPE buffer[MAX_PATH] = { };
	GetModuleFileName(nullptr, buffer, MAX_PATH);
	return getFilePath(normalizeFilePath(ET_STRING_TO_OUTPUT_TYPE(buffer)));
}

bool fileExists(const std::string& name)
{
	DWORD attr = GetFileAttributes(ET_STRING_TO_PARAM_TYPE(name).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
}

bool folderExists(const std::string& folder)
{
	DWORD attr = GetFileAttributes(ET_STRING_TO_PARAM_TYPE(folder).c_str());
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

void findFiles(const std::string& folder, const std::string& mask, bool recursive, StringList& list)
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

			} while (FindNextFile(folderSearch, &folders));
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
		} while (FindNextFile(search, &data));
		FindClose(search);
	}

	if (recursive)
	{
		for (const std::string& i : folderList)
			findFiles(i, mask, recursive, list);
	}
}

std::string applicationPackagePath()
{
	ET_CHAR_TYPE buffer[MAX_PATH] = { };
	GetCurrentDirectory(MAX_PATH, buffer);
	return addTrailingSlash(ET_STRING_TO_OUTPUT_TYPE(buffer));
}

std::string applicationDataFolder()
{
	ET_CHAR_TYPE buffer[MAX_PATH] = { };
	GetCurrentDirectory(MAX_PATH, buffer);
	return addTrailingSlash(ET_STRING_TO_OUTPUT_TYPE(buffer));
}

std::string documentsBaseFolder()
{
	wchar_t* path = nullptr;
	SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

	if (path == nullptr)
		return emptyString;

	std::string result = addTrailingSlash(unicodeToUtf8(path));
	CoTaskMemFree(path);
	return result;
}

std::string libraryBaseFolder()
{
	wchar_t* path = nullptr;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

	if (path == nullptr)
		return emptyString;

	std::string result = addTrailingSlash(unicodeToUtf8(path));
	CoTaskMemFree(path);
	return result;
}

std::string workingFolder()
{
	char buffer[1024] = { };
	GetCurrentDirectoryA(sizeof(buffer) - 2, buffer);

	auto* ptr = buffer;
	while (*++ptr) {}
	*ptr = pathDelimiter;

	return std::string(buffer);
}

bool createDirectory(const std::string& name, bool recursive)
{
	if (recursive)
	{
		char delim[] = { pathDelimiter, 0 };
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

bool removeFile(const std::string& name)
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

bool copyFile(const std::string& from, const std::string& to)
{
	ET_STRING_TYPE aFrom = ET_STRING_TO_PARAM_TYPE(from);
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

bool removeDirectory(const std::string& name)
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

void findSubfolders(const std::string& folder, bool recursive, StringList& list)
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
		} while (FindNextFile(folderSearch, &folders));
		FindClose(folderSearch);
	}

	if (recursive)
	{
		for (const std::string& i : folderList)
			findSubfolders(i, true, list);
	}

	list.insert(list.end(), folderList.begin(), folderList.end());
}

void openUrl(const std::string& url)
{
	ShellExecute(nullptr, ET_STRING_FROM_CONST_CHAR("open"),
		ET_STRING_TO_PARAM_TYPE(url).c_str(), 0, 0, SW_SHOWNORMAL);
}

std::string unicodeToUtf8(const std::wstring& w)
{
	int mbcWidth = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, 0, 0, 0, 0);

	if (mbcWidth == 0)
		return emptyString;

	DataStorage<char> result(mbcWidth + 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, result.data(), static_cast<int>(result.size()), 0, 0);

	return std::string(result.data());
}

std::wstring utf8ToUnicode(const std::string& mbcs)
{
	int uWidth = MultiByteToWideChar(CP_UTF8, 0, mbcs.c_str(), -1, 0, 0);
	if (uWidth == 0)
	{
		switch (GetLastError())
		{
		case ERROR_INSUFFICIENT_BUFFER:
			std::cout << "A supplied buffer size was not large enough, "
				"or it was incorrectly set to NULL." << std::endl;
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

std::string applicationIdentifierForCurrentProject()
{
	return "com.et.app";
}

uint64_t getFileDate(const std::string& path)
{
	WIN32_FIND_DATA findData = { };
	HANDLE search = FindFirstFile(ET_STRING_TO_PARAM_TYPE(path).c_str(), &findData);
	FindClose(search);

	return findData.ftLastWriteTime.dwLowDateTime |
		(static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32);
}

uint64_t getFileUniqueIdentifier(const std::string& path)
{
	WIN32_FIND_DATA findData = {};
	HANDLE search = FindFirstFile(ET_STRING_TO_PARAM_TYPE(path).c_str(), &findData);
	FindClose(search);

	uint64_t dateUid =
		(static_cast<uint64_t>(findData.ftLastWriteTime.dwLowDateTime)) |
		(static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32);

	uint64_t sizeUid =
		(static_cast<uint64_t>(findData.nFileSizeLow)) |
		(static_cast<uint64_t>(findData.nFileSizeHigh) << 32);

	return dateUid ^ sizeUid;
}

Vector<Screen> availableScreens()
{
	Vector<Screen> result;

	EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) -> BOOL
	{
		MONITORINFO info = { sizeof(MONITORINFO) };

		GetMonitorInfo(hMonitor, &info);

		recti screenRect(info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
			info.rcMonitor.bottom - info.rcMonitor.top);

		recti workarea(info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left,
			info.rcWork.bottom - info.rcWork.top);

		Vector<Screen>* r = reinterpret_cast<Vector<Screen>*>(dwData);
		r->emplace_back(screenRect, workarea, 1.0f);
		return 1;
	},
		reinterpret_cast<LPARAM>(&result));

	return result;
}

Screen currentScreen()
{
	return availableScreens().front();
}

std::string selectFile(const StringList& filters, SelectFileMode mode, const std::string& defaultName)
{
	ET_STRING_TYPE defaultFileName = ET_STRING_TO_PARAM_TYPE(defaultName);

	uint32_t fileNameSize = static_cast<uint32_t>(std::max(size_t(MAX_PATH), defaultFileName.size()) + 1, 0);

	StaticDataStorage<ET_CHAR_TYPE, MAX_PATH> fileNameBuffer(0);
	etCopyMemory(fileNameBuffer.begin(), defaultName.c_str(), fileNameSize);

	size_t fp = 0;
	wchar_t fileTypesBuffer[2048] = { };
	for (const std::string& filter : filters)
	{
		std::wstring w = utf8ToUnicode(filter);
		memcpy(fileTypesBuffer + fp, w.data(), w.length() * sizeof(wchar_t));
		fp += 1 + w.length();
	}

	OPENFILENAME of = { };
	of.hwndOwner = reinterpret_cast<HWND>(application().context().objects[0]);
	of.lStructSize = sizeof(of);
	of.hInstance = GetModuleHandle(nullptr);
	of.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
	of.lpstrFile = fileNameBuffer.begin();
	of.nMaxFile = MAX_PATH;
	of.lpstrFilter = fileTypesBuffer;
	of.nFilterIndex = filters.empty() ? 0 : 1;

	auto func = (mode == SelectFileMode::Save) ? GetSaveFileName : GetOpenFileName;
	if (func(&of))
		return ET_STRING_TO_OUTPUT_TYPE(of.lpstrFile);

	return emptyString;
}

void alert(const std::string& title, const std::string& message, const std::string&, AlertType type)
{
	UINT alType = MB_ICONINFORMATION;

	switch (type)
	{
	case AlertType::Warning:
	{
		alType = MB_ICONWARNING;
		break;
	}

	case AlertType::Error:
	{
		alType = MB_ICONERROR;
		break;
	}

	default:
		break;
	}

	MessageBoxA(nullptr, message.c_str(), title.c_str(), alType);
}

}

#endif // ET_PLATFORM_WIN
