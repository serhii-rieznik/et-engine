/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/containers.h>

namespace et
{
	class Locale : public Singleton<Locale>
	{
	public:
		static std::string time();
		static std::string date();
		static size_t currentLocale();
		static std::string localeLanguage(size_t locale);
		static std::string localeSubLanguage(size_t locale);

	public:
		bool loadCurrentLanguageFile(const std::string& rootFolder, const std::string& extension = ".lang");
		bool loadLanguageFile(const std::string& fileName);
		std::string localizedString(const std::string& key);
		
		void printKeyValues();

	private:
		void parseLanguageFile(const std::string& name);
		size_t parseComment(const StringDataStorage& data, size_t index);
		size_t parseKey(const StringDataStorage& data, size_t index);

	private:
		typedef std::map<std::string, std::string> LocaleMap;
		LocaleMap _localeMap;
	};

	std::string localized(const std::string& key);

#	include <et/locale/locale.ext.h>
}