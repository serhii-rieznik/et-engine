/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/containers.h>
#include <et/app/events.h>

namespace et
{
	class Locale : public Singleton<Locale>
	{
	public:
		void setCurrentLocale(const std::string&);
		
		bool resolveLanguageFileName(std::string& fileName, const std::string& rootFolder, const std::string& extension = ".lang");
		bool loadCurrentLanguageFile(const std::string& rootFolder, const std::string& extension = ".lang");
		bool appendCurrentLanguageFile(const std::string& rootFolder, const std::string& extension = ".lang");
		bool loadLanguageFile(const std::string& fileName);
		bool hasKey(const std::string& key);
		
		void appendLocalization(const et::Dictionary&);
		void printContent();

		std::string localizedString(const std::string& key);
		
		const std::string& currentLocale() const
			{ return _currentLocale; }
		
		const Dictionary& localeDictionary() const
			{ return _localeMap; }
		
		ET_DECLARE_EVENT1(localeLoaded, std::string)
		
	private:
		Locale();
		
		Dictionary parseLanguageFile(const std::string& name);
		
		size_t parseComment(const StringDataStorage& data, size_t index);
		size_t parseKey(const StringDataStorage& data, size_t index, Dictionary&);
		
		std::string localizedStringFromObject(const ValueBase::Pointer&, const std::string&);

		ET_SINGLETON_COPY_DENY(Locale)
		
	private:
		Dictionary _localeMap;
		std::string _currentLocale;
	};
	
	namespace locale
	{
		std::string time();
		std::string date();
		std::string localeLanguage(const std::string&);
		std::string localeSubLanguage(const std::string&);
		
		std::string dateTimeFromTimestamp(uint64_t);

		std::string currentLocale();
	}
	
	std::string localized(const std::string& key);
	
#	include <et/locale/locale.ext.h>
}
