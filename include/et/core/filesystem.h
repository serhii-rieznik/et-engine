/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

namespace et
{
	extern const char pathDelimiter;
	extern const char invalidPathDelimiter;
	
	std::string applicationPath();
	std::string applicationPackagePath();
	std::string applicationDataFolder();
	std::string libraryBaseFolder();
	std::string documentsBaseFolder();
	std::string temporaryBaseFolder();
	
	std::string normalizeFilePath(std::string s);
	
	bool fileExists(const std::string& name);
	bool folderExists(const std::string& name);
	
	bool createDirectory(const std::string& name, bool intermediates);
	bool removeDirectory(const std::string& name);
	bool removeFile(const std::string& name);
	bool copyFile(const std::string& fromName, const std::string& toName);
	
	void getFolderContent(const std::string& folder, StringList& list);
	void findFiles(const std::string& folder, const std::string& mask, bool recursive, StringList& list);
	void findSubfolders(const std::string& folder, bool recursive, StringList& list);
	void openUrl(const std::string& url);

	uint64_t getFileDate(const std::string& path);
	
	std::string getFilePath(const std::string& name);
	std::string getFileFolder(const std::string& name);
	std::string getFileName(const std::string& fullPath);
	std::string getFileExt(std::string name);
	
	std::string removeUpDir(std::string name);
	
	std::string replaceFileExt(const std::string& fileName, const std::string& newExt);
	std::string removeFileExt(const std::string& fileName);
	
	std::string addTrailingSlash(const std::string& path);
	
	std::string loadTextFile(const std::string& fileName);
}
