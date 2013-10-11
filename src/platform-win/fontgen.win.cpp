/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Windows.h>

#include <fstream>
#include <et/core/filesystem.h>
#include <et/core/serialization.h>
#include <et/core/tools.h>
#include <et/gui/fontgen.h>

using namespace et;
using namespace et::gui;

const float characterOffset = 2.0f;

const int FontGenVersion = 0x01;

FontGenerator::FontGenerator() : _face("Times New Roman"), _size(12), _offset(0.0f), _outFileSet(false)
{
	for (unsigned short c = 0x20; c < 0x7F; ++c)
		_characterRange.push_back(c);
}

void FontGenerator::fillCharacterDescriptors(const std::string& face, int size, bool bold, CharDescriptorList& chars, 
		const CharacterRange& range, const vec2& extraOffsets)
{
	HDC dc = CreateCompatibleDC(0);
	int pointsSize = MulDiv(size, GetDeviceCaps(dc, LOGPIXELSY), 72);

	HFONT font = CreateFont(-pointsSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face.c_str());

	SelectObject(dc, font);

	for (CharacterRange::const_iterator i = range.begin(), e = range.end(); i != e; ++i)
	{
		SIZE charSize = { };
		wchar_t wStr[2] = { static_cast<wchar_t>(*i), 0 };
		GetTextExtentPointW(dc, wStr, 1, &charSize);

		CharDescriptor desc = CharDescriptor(*i, bold ? CharParameter_Bold : 0);
		desc.size.x = static_cast<float>(charSize.cx + 2) + extraOffsets.x;
		desc.size.y = static_cast<float>(charSize.cy) + extraOffsets.y;
		desc.extra.x = static_cast<int>(0.5f * extraOffsets.x + 1.0f);
		desc.extra.y = static_cast<int>(0.5f * extraOffsets.y - 1.0f);
		desc.extra.z = reinterpret_cast<int>(font);

		chars.push_back(desc);
	}

	DeleteDC(dc);
}

FontGeneratorResult FontGenerator::generate(ImageFormat fmt)
{
	if (!_outFileSet) 
		return FontGeneratorResult_OutputFileNotDefined;

	std::string _outInfoFile = _outFile + ".font";
	std::string _outLayoutFile = _outFile + ".layout" + ImageWriter::extensionForImageFormat(fmt);
	std::string _outFontFile = _outFile + ImageWriter::extensionForImageFormat(fmt);

	CharDescriptorList chars;
	fillCharacterDescriptors(_face, _size, false, chars, _characterRange, vec2(_offset));
	fillCharacterDescriptors(_face, _size, true, chars, _characterRange, vec2(_offset));

	int charsPerRow = static_cast<int>(::sqrt(static_cast<float>(chars.size()))) * 2;

	vec2i textureSize;
	float textureWidth = 0.0f;

	int index = 0;
	float x_pos = characterOffset;
	for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i, ++index)
	{
		x_pos += i->size.x + characterOffset;
		if (index && (index % charsPerRow == 0))
		{
			textureWidth = etMax(x_pos, textureWidth);
			x_pos = characterOffset;
		}
	}

	textureWidth = etMin(1024.0f, textureWidth);

	bool foundOptimal = false;
	while (!foundOptimal)
	{
		float lineHeight = 0.0f;
		float y_pos = characterOffset;
		x_pos = characterOffset;
		textureSize.x = roundToHighestPowerOfTow(static_cast<size_t>(textureWidth));
		textureWidth = static_cast<float>(textureSize.x);
		for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i)
		{
			if (x_pos + i->size.x + characterOffset >= textureSize.x)
			{
				x_pos = characterOffset;
				y_pos += lineHeight + characterOffset;
				lineHeight = 0.0f;
			}

			i->origin = vec2(x_pos, y_pos);
			x_pos += i->size.x + characterOffset;
			lineHeight = etMax(i->size.y, lineHeight);
		}
		textureSize.y = roundToHighestPowerOfTow( static_cast<size_t>(y_pos + lineHeight + characterOffset) );

		foundOptimal = textureSize.y / textureSize.x <= 1;

		if (foundOptimal) break;

		textureSize.x *= 2;
		textureWidth = static_cast<float>(textureSize.x);
	}

	for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i)
	{
		i->uvOrigin.x = i->origin.x / static_cast<float>(textureSize.x);
		i->uvOrigin.y = 1.0f - i->origin.y / static_cast<float>(textureSize.y);
		i->uvSize.x = i->size.x / static_cast<float>(textureSize.x);
		i->uvSize.y = i->size.y / static_cast<float>(textureSize.y);
	}

	int dataSize = textureSize.square() * 4;

	BITMAPINFO bi = { sizeof(BITMAPINFO) };
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biHeight = textureSize.y;
	bi.bmiHeader.biWidth = textureSize.x;
	bi.bmiHeader.biSizeImage = dataSize;

	BinaryDataStorage data(dataSize);
	data.fill(0);

	HBITMAP bitmap = CreateBitmap(textureSize.x, textureSize.y, 1, 32, 0);

	HDC dc = CreateCompatibleDC(0);
	SelectObject(dc, bitmap);
	SetTextColor(dc, 0xffffff);
	SetBkMode(dc, TRANSPARENT);
	SetDIBits(dc, bitmap, 0, textureSize.y, data.data(), &bi, DIB_RGB_COLORS);

	for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i)
	{
		wchar_t wStr[2] = { static_cast<wchar_t>(i->value), 0 };
		SelectObject(dc, reinterpret_cast<HFONT>(i->extra.z));

		TextOutW(dc, static_cast<int>(i->origin.x) + i->extra.x, 	static_cast<int>(i->origin.y) + i->extra.y, wStr, 1);
	}

	GetDIBits(dc, bitmap, 0, textureSize.y, data.data(), &bi, DIB_RGB_COLORS);

	for (int i = 0; i < dataSize / 4; ++i)
	{
		unsigned char r = data[4*i+0];
		unsigned char g = data[4*i+1];
		unsigned char b = data[4*i+2];
		data[4*i+0] = 255;
		data[4*i+1] = 255;
		data[4*i+2] = 255;
		data[4*i+3] = (76 * r + 151 * g + 28 * b) / 255;
	}

	FontGeneratorResult result = FontGeneratorResult_Success;

	if (!ImageWriter::writeImageToFile(_outFontFile, data, textureSize, 4, 8, fmt, false))
		result = FontGeneratorResult_OutputFileFailed;

	data.fill(0);

	for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i)
	{
		unsigned char rndColor[3] = { 128 + rand() % 127, 128 + rand() % 127, 128 + rand() % 127 };

		int x = static_cast<int>(i->origin.x);
		int y = static_cast<int>(i->origin.y);
		int w = static_cast<int>(i->size.x);
		int h = static_cast<int>(i->size.y);
		for (int v = 0; v < h; ++v)
		{
			for (int u = 0; u < w; ++u)
			{
				int index = (x + u) + (textureSize.y - 1 - y - v) * textureSize.x;
				data[4 * index + 0] = rndColor[0];
				data[4 * index + 1] = rndColor[1];
				data[4 * index + 2] = rndColor[2];
				data[4 * index + 3] = 255;
			}
		}
	}

	if (!ImageWriter::writeImageToFile(_outLayoutFile, data, textureSize, 4, 8, fmt, false))
		result = FontGeneratorResult_OutputFileFailed;

	DeleteObject(bitmap);
	DeleteDC(dc);

	std::ofstream fontFile(_outInfoFile, std::ios::binary);
	if (fontFile.fail())
		return FontGeneratorResult_OutputFileFailed;

	serializeInt(fontFile, FontGenVersion);
	serializeString(fontFile, _face);
	serializeInt(fontFile, _size);
	serializeString(fontFile, getFileName(_outFontFile));
	serializeString(fontFile, getFileName(_outLayoutFile));
	serializeInt(fontFile, chars.size());
	for (CharDescriptorList::iterator i = chars.begin(), e = chars.end(); i != e; ++i)
	{
		CharDescriptor& desc = *i;
		fontFile.write(reinterpret_cast<const char*>(&desc), sizeof(CharDescriptor));
	}
	fontFile.close();


	return result;
}

void FontGenerator::setOutputFile(const std::string& fileName)
{
	_outFile = fileName;
	_outFileSet = true;
}
