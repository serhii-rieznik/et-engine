/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/core/tools.h>
#include <et/core/filesystem.h>
#include <et/core/stream.h>
#include <et/locale/locale.h>

using namespace et;

const char CommentChar = '/';
const char KeyChar = '\"';
const char NewLineChar = '\n';

std::string Locale::localeLanguage(size_t locale)
{
	char lang[3] = { static_cast<char>(locale & 0x000000ff),
		static_cast<char>((locale & 0x0000ff00) >> 8), 0 };
	
	return std::string(lang);
}

std::string Locale::localeSubLanguage(size_t locale)
{
	char lang[3] = { static_cast<char>((locale & 0x00ff0000) >> 16),
		static_cast<char>((locale & 0xff000000) >> 24), 0 };
	
	return std::string(lang);
}

bool Locale::loadLanguageFile(const std::string& fileName)
{
	if (fileExists(fileName))
	{
		parseLanguageFile(fileName);
		return true;
	}

	std::cout << "Unable to load language file `" << fileName << "`" << std::endl;
	return false;
}

bool Locale::loadCurrentLanguageFile(const std::string& rootFolder, const std::string& extension)
{
	size_t cLocale = Locale::currentLocale();
	std::string lang = Locale::localeLanguage(cLocale);
	std::string fileName = addTrailingSlash(rootFolder) + lang + extension;
	if (fileExists(fileName))
	{
		parseLanguageFile(fileName);
		return true;
	}
	else
	{
		std::string subLang = Locale::localeSubLanguage(cLocale);
		fileName = addTrailingSlash(rootFolder) + subLang + extension;
		if (fileExists(fileName))
		{
			parseLanguageFile(fileName);
			return true;
		}
		else
		{
			fileName = addTrailingSlash(rootFolder) + "en" + extension;
			if (fileExists(fileName))
			{
				parseLanguageFile(fileName);
				return true;
			}
		}
	}
	
	std::cout << "Unable to locate language file `" << fileName << "` in folder " << rootFolder << std::endl;
	return false;
}

std::string Locale::localizedString(const std::string& key)
{
	auto i = _localeMap.find(key);
	return (i == _localeMap.end()) ? key : i->second;
}

void Locale::parseLanguageFile(const std::string& fileName)
{
	_localeMap.clear();

	InputStream file(fileName, StreamMode_Binary);
	if (file.invalid()) return;

	StringDataStorage raw(streamSize(file.stream()) + 1, 0);
	file.stream().read(raw.data(), raw.size());

	StringDataStorage keyValues(raw.size(), 0);

	size_t i = 0;
	bool inQuote = false;
	while ((i < raw.size()) && raw[i])
	{
		size_t prevChar = i > 0 ? i - 1 : 0;
		if ((raw[i] == CommentChar) && (raw[prevChar] == NewLineChar))
		{
			inQuote = false;
			i = parseComment(raw, i+1);
		}
		else 
		{
			if (raw[i] == KeyChar)
				inQuote = !inQuote;

			if (inQuote || !isWhitespaceChar(raw[i]))
				keyValues.push_back(raw[i]);
			
			++i;
		}
	}

	inQuote = false;
	size_t sourceLength = keyValues.lastElementIndex();
	StringDataStorage source(sourceLength + 1, 0);
	for (size_t j = 0; j < sourceLength; ++j)
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
		i = (source[i] == KeyChar) ? parseKey(source, i+1) : ++i;
}

size_t Locale::parseKey(const StringDataStorage& data, size_t index)
{
	size_t keyEnd = 0;
	size_t valueStart = 0;
	size_t i = index;
	
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

	size_t keyLenght = keyEnd - index;
	size_t valueLenght = i - valueStart - 1;
	
	std::string key(keyLenght, 0);
	for (size_t j = 0; j < keyLenght; ++j)
		key[j] = data[index+j];
	
	index = 0;
	std::string value(valueLenght, 0);
	for (size_t j = 0; j < valueLenght; ++j)
	{
		if ((data[valueStart+j] == '\\') && (i+1 < valueLenght))
		{
			++j;
			if (data[valueStart+j] == 'n')
				value[index] = 0x0a;
			else if (data[valueStart+j] == '\\')
				value[index] = '\\';
			else
				std::cout << "Unsupported sequence: \\" << data[valueStart+j] << std::endl;
		}
		else
		{
			value[index] = data[valueStart+j];
		}
		
		++index;
	}
	value.resize(index);
	
	_localeMap[key] = value;
	
	return i + 1;
}

size_t Locale::parseComment(const StringDataStorage& data, size_t index)
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
		std::cout << "Unsupported comment format in language file";
		return index;
	}
}

void Locale::printKeyValues()
{
	for (auto& i : _localeMap)
	{
		const std::string& key = i.first;
		std::string& value = i.second;
		std::cout << key.size() << " / " << value.size() << " -> " << key << " -> " << value << std::endl;
		std::cout.flush();
	}
}

std::string et::localized(const std::string& key)
{
	return Locale::instance().localizedString(key); 
}
