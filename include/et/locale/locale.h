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
		void setCurrentLocale(const std::string&);
		
		bool loadCurrentLanguageFile(const std::string& rootFolder, const std::string& extension = ".lang");
		bool loadLanguageFile(const std::string& fileName);
		
		std::string localizedString(const std::string& key);
		
		void printContent();

	private:
		Locale();
		
		void parseLanguageFile(const std::string& name);
		
		size_t parseComment(const StringDataStorage& data, size_t index);
		size_t parseKey(const StringDataStorage& data, size_t index);

		ET_SINGLETON_COPY_DENY(Locale)
		
	private:
		typedef std::map<std::string, std::string> LocaleMap;
		LocaleMap _localeMap;
		size_t _locale;
	};
	
	namespace locale
	{
		std::string time();
		std::string date();
		std::string localeLanguage(size_t locale);
		std::string localeSubLanguage(size_t locale);
		
		size_t currentLocale();
		size_t localeToIdentifier(const std::string&);
	}
	
	std::string localized(const std::string& key);
	
#	include <et/locale/locale.ext.h>
}