/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/et.h>
#include <et/core/serialization.h>
#include <et/core/conversion.h>
#include <et/app/application.h>
#include <et/gui/font.h>

using namespace et;
using namespace et::gui;

FontData::FontData() : _size(0)
{
}

FontData::FontData(RenderContext* rc, const std::string& fileName, ObjectsCache& cache) :  _size(0)
{
	loadFromFile(rc, fileName, cache);
}

FontData::FontData(const CharacterGenerator::Pointer& generator) : _generator(generator), _size(0)
{
}

FontData::~FontData()
{

}

void FontData::loadFromFile(RenderContext* rc, const std::string& fileName, ObjectsCache& cache)
{
	_chars.clear();
	_boldChars.clear();
	_texture = Texture();

	std::string resolvedFileName =
		application().environment().resolveScalableFileName(fileName, rc->screenScaleFactor());

	InputStream fontFile(resolvedFileName, StreamMode_Binary);
	if (fontFile.invalid()) return;

	std::string fontFileDir = getFilePath(resolvedFileName);

	int version = deserializeInt(fontFile.stream());
	_face = deserializeString(fontFile.stream());
	_size = deserializeUInt(fontFile.stream());
	
	std::string textureFile = deserializeString(fontFile.stream());
	std::string layoutFile = deserializeString(fontFile.stream());
	std::string textureFileName = fontFileDir + textureFile;
	std::string actualName = fileExists(textureFileName) ? textureFileName : textureFile;

	_texture = rc->textureFactory().loadTexture(actualName, cache);
	_biggestChar = vec2(0.0f);
	_biggestBoldChar = vec2(0.0f);
	int charCount = deserializeInt(fontFile.stream());

	if (version == FONT_VERSION_1)
	{
		for (int i = 0; i < charCount; ++i)
		{
			CharDescriptor desc;
			char* ptr = reinterpret_cast<char*>(&desc);
			ptr += 2 * sizeof(int);
			
			unsigned short sValue = 0;
			unsigned short sParams = 0;
			fontFile.stream().read(reinterpret_cast<char*>(&sValue), sizeof(sValue));
			fontFile.stream().read(reinterpret_cast<char*>(&sParams), sizeof(sParams));
			fontFile.stream().read(ptr, sizeof(desc) - 2 * sizeof(int));

			desc.value = sValue;
			desc.params = sParams;
			
			if ((desc.params & CharParameter_Bold) == CharParameter_Bold)
			{
				_biggestBoldChar = maxv(_biggestBoldChar, desc.size);
				_boldChars[desc.value] = desc;
			}
			else
			{
				_biggestChar = maxv(_biggestChar, desc.size);
				_chars[desc.value] = desc;
			}
		}
	}
	else if (version == FONT_VERSION_2)
	{
		for (int i = 0; i < charCount; ++i)
		{
			CharDescriptor desc;
			fontFile.stream().read(reinterpret_cast<char*>(&desc), sizeof(desc));
			
			if ((desc.params & CharParameter_Bold) == CharParameter_Bold)
			{
				_biggestBoldChar = maxv(_biggestBoldChar, desc.size);
				_boldChars[desc.value] = desc;
			}
			else
			{
				_biggestChar = maxv(_biggestChar, desc.size);
				_chars[desc.value] = desc;
			}
		}
	}
}

CharDescriptor FontData::charDescription(int c)
{
	if (_generator.valid())
		return _generator->charDescription(c);

	auto i = _chars.find(c);
	return (i != _chars.end()) ? i->second : CharDescriptor(c, 0, _biggestChar * vec2(0.0f, 1.0f));
}

CharDescriptor FontData::boldCharDescription(int c)
{
	if (_generator.valid())
		return _generator->boldCharDescription(c);

	auto i = _boldChars.find(c);

	return (i != _boldChars.end()) ? i->second :
		CharDescriptor(c, CharParameter_Bold, _biggestBoldChar * vec2(0.0f, 1.0f));
}

float FontData::lineHeight() const
{
	if (_generator.valid())
		return _generator->lineHeight();
	else 
		return _chars.size() ? _chars.begin()->second.size.y : static_cast<float>(_size);
}

vec2 FontData::measureStringSize(const CharDescriptorList& s)
{
	vec2 sz;
	vec2 lineSize;

	for (auto& desc : s)
	{
		lineSize.y = etMax(lineSize.y, desc.size.y);
		
		if ((desc.value == ET_RETURN) || (desc.value == ET_NEWLINE))
		{
			sz.x = etMax(sz.x, lineSize.x);
			sz.y += lineSize.y;
			lineSize = vec2(0.0f);
		}
		else 
		{
			lineSize.x += desc.size.x;
		}
	}
	
	sz.x = etMax(lineSize.x, sz.x);
	sz.y += lineSize.y;
	
	return sz;
}

vec2 FontData::measureStringSize(const std::string& s, bool formatted)
{
	if (isUtf8String(s))
	{
		std::wstring ws = utf8ToUnicode(s);
		return measureStringSize(formatted ? parseString(ws) : buildString(ws, formatted));
	}
	
	return measureStringSize(formatted ? parseString(s) : buildString(s, formatted));
}

vec2 FontData::measureStringSize(const std::wstring& s, bool formatted)
{
	return measureStringSize(formatted ? parseString(s) : buildString(s, formatted));
}

CharDescriptorList FontData::buildString(const std::string& s, bool formatted)
{
	if (isUtf8String(s))
		return buildString(utf8ToUnicode(s), formatted);

	if (formatted)
		return parseString(s);

	CharDescriptorList result;
	for (const auto& i : s)
		result.push_back(charDescription(i));
	return result;
}

CharDescriptorList FontData::buildString(const std::wstring& s, bool formatted)
{
	if (formatted)
		return parseString(s);

	CharDescriptorList result;
	for (const auto& i : s)
		result.push_back(charDescription(i));
	return result;
}

CharDescriptorList FontData::parseString(const std::string& s)
{
	CharDescriptorList result;
	
	const char tagOpening = '<';
	const char tagClosing = '>';
	const char tagClosingId = '/';
	const std::string tagBold("b");
	const std::string tagColor("color");
	const std::string tagColorOpen("color=#");

	std::deque<vec4> colors;
	colors.push_front(vec4(1.0f));

	int nBoldTags = 0;
	int nColorTags = 0;
	bool readingTag = false;
	bool closingTag = false;
	std::string tag;

	for (auto& c : s)
	{
		if (c == tagOpening)
		{
			readingTag = true;
			closingTag = false;
			tag = std::string();
		}
		else if (c == tagClosing)
		{
			if (readingTag)
			{
				if (closingTag)
				{
					if (tag == tagBold)
					{
						if (nBoldTags)
							--nBoldTags;
					}
					else if (tag.find_first_of(tagColor) == 0)
					{
						if (nColorTags)
						{
							--nColorTags;
							colors.pop_front();
						}
					}
					else 
					{
						log::warning("Unknown tag `%s` passed in string: %s", tag.c_str(), s.c_str());
					}
				}
				else
				{
					if (tag == tagBold)
					{
						nBoldTags++;
					}
					else if (tag.find(tagColorOpen) == 0)
					{
						nColorTags++;
						tag.erase(0, tagColorOpen.size());
						colors.push_front(strHexToVec4(tag));
					}
					else
					{
						log::warning("Unknown tag `%s` passed in string: %s", tag.c_str(), s.c_str());
					}
				}
				readingTag = false;
				closingTag = false;
			}
		}
		else 
		{
			if (readingTag)
			{
				if (c == tagClosingId)
				{
					closingTag = true;
				}
				else if (!isWhitespaceChar(c))
				{
					tag += c;
				}
			}
			else
			{
				result.push_back(nBoldTags ? boldCharDescription(c) : charDescription(c));
				result.back().color = colors.front();
			}
		}
	}

	return result;
}

CharDescriptorList FontData::parseString(const std::wstring& s)
{
	CharDescriptorList result;
	
	const wchar_t tagOpening = L'<';
	const wchar_t tagClosing = L'>';
	const wchar_t tagClosingId = L'/';
	const std::wstring tagBold(L"b");
	const std::wstring tagColor(L"color");
	const std::wstring tagColorOpen(L"color=#");

	std::deque<vec4> colors;
	colors.push_front(vec4(1.0f));

	int nBoldTags = 0;
	int nColorTags = 0;
	bool readingTag = false;
	bool closingTag = false;
	std::wstring tag;

	for (auto& c : s)
	{
		if (c == tagOpening)
		{
			readingTag = true;
			closingTag = false;
			tag = std::wstring();
		}
		else if (c == tagClosing)
		{
			if (readingTag)
			{
				if (closingTag)
				{
					if (tag == tagBold)
					{
						if (nBoldTags)
							--nBoldTags;
					}
					else if (tag.find_first_of(tagColor) == 0)
					{
						if (nColorTags)
						{
							--nColorTags;
							colors.pop_front();
						}
					}
					else 
					{
						log::warning("Unknown tag `%S` passed in string: %S", tag.c_str(), s.c_str());
					}
				}
				else
				{
					if (tag == tagBold)
					{
						nBoldTags++;
					}
					else if (tag.find(tagColorOpen) == 0)
					{
						nColorTags++;
						tag.erase(0, tagColorOpen.size());
						colors.push_front(strHexToVec4(tag));
					}
					else
					{
						log::warning("Unknown tag `%S` passed in string: %S", tag.c_str(), s.c_str());
					}
				}
				readingTag = false;
				closingTag = false;
			}
		}
		else 
		{
			if (readingTag)
			{
				if (c == tagClosingId)
				{
					closingTag = true;
				}
				else if (!isWhitespaceChar(c))
				{
					tag += c;
				}
			}
			else
			{
				result.push_back(nBoldTags ? boldCharDescription(c) : charDescription(c));
				result.back().color = colors.front();
			}
		}
	}

	return result;
}

bool FontData::isUtf8String(const std::string& s) const
{
	for (auto c : s)
	{
		if (c & 0x80)
			return true;
	}
	
	return false;
}