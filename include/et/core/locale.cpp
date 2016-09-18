/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/tools.h>
#include <et/locale/locale.hpp>
#include <et/core/json.h>

using namespace et;

const char CommentChar = '/';
const char KeyChar = '\"';
const char NewLineChar = '\n';
const std::string kDefaultLocale = "en";

Locale::Locale() :
	_currentLocale(locale::currentLocale())
{
}

bool Locale::loadLanguageFile(const std::string& fileName)
{
	if (fileExists(fileName))
	{
		parseLanguageFile(fileName);
		localeLoaded.invokeInMainRunLoop(_currentLocale);
		return true;
	}

	log::error("Unable to load language file: %s", fileName.c_str());
	return false;
}

bool Locale::resolveLanguageFileName(std::string& fileName, const std::string& rootFolder, const std::string& extension)
{
	std::string basePath = addTrailingSlash(rootFolder);
	std::string lang = locale::localeLanguage(_currentLocale);
	std::string subLang = locale::localeSubLanguage(_currentLocale);
	
	if (!subLang.empty())
	{
		fileName = basePath + lang + "-" + subLang + extension;
		if (fileExists(fileName))
			return true;
	}
	
	fileName = basePath + lang + extension;
	if (fileExists(fileName))
		return true;
	
	if (lang != kDefaultLocale)
	{
		fileName = addTrailingSlash(rootFolder) + kDefaultLocale + extension;
		return fileExists(fileName);
	}
	
	return false;
}

bool Locale::loadCurrentLanguageFile(const std::string& rootFolder, const std::string& extension)
{
	std::string fileName;
	if (resolveLanguageFileName(fileName, rootFolder, extension))
	{
		_localeMap = parseLanguageFile(fileName);
		return true;
	}
	return false;
}

bool Locale::appendCurrentLanguageFile(const std::string& rootFolder, const std::string& extension)
{
	std::string fileName;
	if (resolveLanguageFileName(fileName, rootFolder, extension))
	{
		appendLocalization(parseLanguageFile(fileName));
		return true;
	}
	return false;
}

std::string Locale::localizedStringFromObject(const VariantBase::Pointer& obj, const std::string& def)
{
	if (obj->variantClass() == VariantClass::String)
	{
		return StringValue(obj)->content;
	}
	else if (obj->variantClass() == VariantClass::Array)
	{
		ArrayValue arr(obj);
		return arr->content.empty() ? def : localizedStringFromObject(arr->content.front(), def);
	}
	
	log::error("Invalid locale key %s, of type: %d", def.c_str(), obj->variantClass());
	
	return def;
}

bool Locale::hasKey(const std::string& key)
{
	return _localeMap->content.count(key) > 0;
}

std::string Locale::localizedString(const std::string& key)
{
	return (!key.empty() && hasKey(key)) ?
		localizedStringFromObject(_localeMap.objectForKey(key), key) : key;
}

et::Dictionary Locale::parseLanguageFile(const std::string& fileName)
{
	et::Dictionary result;
	
	StringDataStorage fileContent;
	{
		InputStream file(fileName, StreamMode_Binary);
		if (file.valid())
		{
			fileContent.resize(streamSize(file.stream()) + 1);
			fileContent.fill(0);
			file.stream().read(fileContent.data(), fileContent.size());
		}
	}
	
	if (fileContent.size() == 0)
		return result;

	/*
	 * Try to parse JSON
	 */
	VariantClass vc = VariantClass::Invalid;
	auto object = json::deserialize(fileContent.binary(), vc, false);
	if (vc == VariantClass::Dictionary)
		return object;
	
	/*
	 * Parse custom format
	 */
	StringDataStorage keyValues(fileContent.size(), 0);
	uint32_t i = 0;
	bool inQuote = false;
	while ((i < fileContent.size()) && fileContent[i])
	{
		uint32_t prevChar = i > 0 ? i - 1 : 0;
		if ((fileContent[i] == CommentChar) && (fileContent[prevChar] == NewLineChar))
		{
			inQuote = false;
			i = parseComment(fileContent, i+1);
		}
		else 
		{
			if (fileContent[i] == KeyChar)
				inQuote = !inQuote;

			if (inQuote || !isWhitespaceChar(fileContent[i]))
				keyValues.push_back(fileContent[i]);
			
			++i;
		}
	}

	inQuote = false;
	uint32_t sourceLength = keyValues.lastElementIndex();
	StringDataStorage source(sourceLength + 1, 0);
	for (uint32_t j = 0; j < sourceLength; ++j)
	{
		char c = keyValues[j];

		if (c == KeyChar) 
			inQuote = !inQuote;

		bool isWhiteSpace = !inQuote && ((c == 0x20) || (c == 0x09));
		bool isNewLine = (c == 0x0a) || (c == 0x0d);
		bool hasNextQuoteMark = (j + 1 < sourceLength) && (keyValues[j+1] == KeyChar);
		bool hasPrevQuoteMark = (j > 0) && (keyValues[j-1] == KeyChar);
		
		bool shouldConcatMultiline =
			((c == KeyChar) && hasNextQuoteMark) || ((c == KeyChar) && hasPrevQuoteMark);

		if (isWhiteSpace || isNewLine || shouldConcatMultiline) continue;

		source.push_back(c);
	}
	
	i = 0;
	while (source[i] && (i < source.size()) && source[i+1])
		i = (source[i] == KeyChar) ? parseKey(source, i+1, result) : ++i;
	
	return result;
}

uint32_t Locale::parseKey(const StringDataStorage& data, uint32_t index, Dictionary& values)
{
	uint32_t keyEnd = 0;
	uint32_t valueStart = 0;
	uint32_t i = index;
	
	while ((i < data.size()) && data[i] && (data[i] != ';'))
	{
		if (data[i] == KeyChar)
		{
			if (keyEnd == 0)
				keyEnd = i;
			
			else if (valueStart == 0)
				valueStart = i+1;
		}
		++i;
	}

	uint32_t keyLenght = keyEnd - index;
	uint32_t valueLenght = i - valueStart - 1;
	
	std::string key(keyLenght, 0);
	for (uint32_t j = 0; j < keyLenght; ++j)
		key[j] = data[index+j];
	
	index = 0;
	std::string value(valueLenght, 0);
	for (uint32_t j = 0; j < valueLenght; ++j)
	{
		if ((data[valueStart+j] == '\\') && (j + 1 < valueLenght))
		{
			++j;
			if (data[valueStart+j] == 'n')
				value[index] = 0x0a;
			else if (data[valueStart+j] == '\\')
				value[index] = '\\';
			else if (data[valueStart+j] == '"')
				value[index] = '"';
			else
				log::error("Unsupported sequence in locale file: \\%c", data[valueStart+j]);
		}
		else
		{
			value[index] = data[valueStart+j];
		}
		
		++index;
	}
	value.resize(index);
	
	values.setStringForKey(key, value);
	
	return i + 1;
}

uint32_t Locale::parseComment(const StringDataStorage& data, uint32_t index)
{
	if (data[index] == CommentChar) // line comment
	{
		while ((index < data.size()) && data[index] && !((data[index] == 0x0a)||(data[index] == 0x0d)))
			++index;
		return index + 1;
	}
	else if (data[index] == '*') // block comment
	{
		while ((index + 1 < data.size()) && data[index] && !((data[index] == '*') && (data[index+1] == CommentChar)) )
			++index;
		
		return index + 2;
	}
	else
	{
		log::error("Unsupported comment format in language file");
		return index;
	}
}

void Locale::setCurrentLocale(const std::string& l)
{
	_currentLocale = l;
}

void Locale::appendLocalization(const et::Dictionary& l)
{
	for (const auto& o : l->content)
		_localeMap.setObjectForKey(o.first, o.second);
}

void Locale::printContent()
{
	log::info("Locale:");
	
	for (const auto& i : _localeMap->content)
		log::info("%s = %s", i.first.c_str(), StringValue(i.second)->content.c_str());
}

std::string et::localized(const std::string& key)
{
	return Locale::instance().localizedString(key); 
}

std::string locale::localeLanguage(const std::string& key)
{
	auto dashPos = key.find('-');
	return (dashPos == std::string::npos) ? key : key.substr(0, dashPos);
}

std::string locale::localeSubLanguage(const std::string& key)
{
	auto dashPos = key.find('-');
	return (dashPos == std::string::npos) ? emptyString : key.substr(dashPos + 1);
}
