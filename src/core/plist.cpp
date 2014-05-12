/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/plist.h>
#include <et/core/tools.h>

using namespace et;

static const std::string keyOpen = "<key>";
static const std::string keyClose = "</key>";
static const std::string keyDictionaryOpen = "<dict>";
static const std::string keyDictionaryClose = "</dict>";
static const std::string keyArrayOpen = "<array>";
static const std::string keyArrayClose = "</array>";
static const std::string keyStringOpen = "<string>";
static const std::string keyStringClose = "</string>";
static const std::string keyBoolTrue = "<true/>";
static const std::string keyBoolFalse = "<false/>";
static const std::string keyIntegerOpen = "<integer>";
static const std::string keyIntegerClose = "</integer>";
/*
DictionaryEntry<Dictionary>::Pointer Reader::load(const std::string& filename)
{
	DictionaryEntry<Dictionary>::Pointer result(new DictionaryEntry<Dictionary>);
	
	InputStream file(filename, StreamMode_Binary);
	if (file.invalid())
		return result;
	
	BinaryDataStorage buffer(streamSize(file.stream()) + 1, 0);
	file.stream().read(buffer.binary(), buffer.size() - 1);
	parseBuffer(buffer, result.ptr());

	return result;
}

inline bool isEndLine(char c)
	{ return (c == 10) || (c == 13); }

inline size_t isEndLineChars(char* c)
{
	size_t firstChar = isEndLine(c[0]) ? 1 : 0;
	size_t secChar = isEndLine(c[1]) ? 1 : 0;
	return firstChar * (1 + secChar);
}

inline bool isOpeningTag(char c)
	{ return c == charOpeningTag; }

inline bool isClosingTag(char c)
	{ return c == charClosingTag; }

inline size_t distanceToChar(char* ptr, char value, bool including)
{
	size_t result = including ? 1 : 0;
	while (*ptr++ != value) ++result;
	return result;
}

inline std::string extractString(char*& ptr, size_t from, size_t len, bool offsetPtr)
{
	std::string value;
	value.resize(len);

	for (size_t i = 0; i < len; ++i, ptr += offsetPtr ? 1 : 0)
		value[i] = offsetPtr ? ptr[from] : ptr[from + i];

	return value;
}

inline std::string pickTag(char*& ptr)
{
	return extractString(ptr, 0, distanceToChar(ptr, charClosingTag, true), false);
}

inline std::string readTag(char*& ptr)
{
	return extractString(ptr, 0, distanceToChar(ptr, charClosingTag, true), true);
}

inline std::string pickValue(char*& ptr)
{
	return extractString(ptr, 0, distanceToChar(ptr, charOpeningTag, false), false);
}

inline std::string readValue(char*& ptr)
{
	return extractString(ptr, 0, distanceToChar(ptr, charOpeningTag, false), true);
}

inline bool readKey(char*& ptr, std::string& name)
{
	if (!isOpeningTag(ptr[0])) return false;

	std::string key = extractString(ptr, 0, distanceToChar(ptr, charClosingTag, true), true);
	if (key != keyOpen) return false;

	name = readValue(ptr);
	key = extractString(ptr, 0, distanceToChar(ptr, charClosingTag, true), true);

	return (key == keyClose);
}

void Reader::parseBuffer(BinaryDataStorage& buffer, DictionaryEntry<Dictionary>* root)
{
	char* ptr = buffer.binary();

	// hack: assuming first 3 rows contains unused data
	size_t lines = 0;
	while (*ptr && (lines < 3))
	{
		size_t el = isEndLineChars(ptr);  
		ptr += el ? el : 1;
		lines += el ? 1 : 0;
	}

	if (*ptr == 0)
	{
		ET_ASSERT(false && "Unable to parse plist");
		return;
	}

	BinaryDataStorage cleanedUp(buffer.size(), 0);
	while (*ptr)
	{
		if (!isWhitespaceChar(ptr[0]))
			cleanedUp.push_back(ptr[0]);
		ptr++;
	}
	ptr = cleanedUp.binary();

	std::string tag = readTag(ptr);
	if (tag != keyDictionaryOpen)
	{
		ET_ASSERT(false && "Unable to parse plist");
		return;
	}

	parseDictionary(ptr, root);
}

bool Reader::parseDictionary(char*& ptr, DictionaryEntry<Dictionary>* owner)
{
	if (!isOpeningTag(ptr[0])) 
	{
		ET_ASSERT(false && "Unable to parse plist");
		return false;
	}

	volatile bool reading = true;
	while (*ptr && reading)
	{
		std::string key;
		if (!readKey(ptr, key))
		{
			ET_ASSERT(false && "Unable to parse plist");
			return false;
		}

		DictionaryEntryBase* val = 0;
		if (parseValue(ptr, &val))
		{
			owner->setValueForKey(DictionaryEntryBase::Pointer(val), key);
		}
		else
		{
			log::error("Unable to read value for key: %s", key.c_str());
			ET_ASSERT(false);
		}

		if (pickTag(ptr) == keyDictionaryClose)
		{
			readTag(ptr);
			reading = false;
		}
	}

	return true;
}

bool Reader::parseString(char*& ptr, DictionaryEntryBase** value)
{
	std::string v = readValue(ptr);

	if (readTag(ptr) != keyStringClose) return false;
	*value = new DictionaryEntry<std::string>(v);
	
	return true;
}

bool Reader::parseInteger(char*& ptr, DictionaryEntryBase** value)
{
	std::string v = readValue(ptr);

	if (readTag(ptr) != keyIntegerClose) return false;
	*value = new DictionaryEntry<double>(strToDouble(v));

	return true;
}

bool Reader::parseValue(char*& ptr, DictionaryEntryBase** value)
{
	std::string valueType = readTag(ptr);

	if (valueType == keyDictionaryOpen)
	{
		*value = new DictionaryEntry<Dictionary>;
		return parseDictionary(ptr, static_cast<DictionaryEntry<Dictionary>*>(*value));
	}
	else if (valueType == keyArrayOpen)
	{
		return parseArray(ptr, value);
	}
	else if (valueType == keyStringOpen)
	{
		return parseString(ptr, value);
	}
	else if (valueType == keyIntegerOpen)
	{
		return parseInteger(ptr, value);
	}
	else if (valueType == keyBoolTrue)
	{
		*value = new DictionaryEntry<bool>(true);
		return true;
	}
	else if (valueType == keyBoolFalse)
	{
		*value = new DictionaryEntry<bool>(false);
		return true;
	}
	else
	{
		log::error("Unable to read value of type: %s", valueType.c_str());
		ET_ASSERT(false);
	}
	return false;
}

bool Reader::parseArray(char*& ptr, DictionaryEntryBase** value)
{
	DictionaryEntry<Array>* aValue = new DictionaryEntry<Array>();
	*value = aValue;

	volatile bool reading = true;
	while (*ptr && reading)
	{
		std::string nextTag = pickTag(ptr);
		if (nextTag == keyArrayClose)
		{
			readTag(ptr);
			reading = false;
		}
		else
		{
			DictionaryEntryBase* v = 0;
			if (parseValue(ptr, &v))
				aValue->push_back(DictionaryEntryBase::Pointer(v));
		}

	}

	return true;
}
*/
