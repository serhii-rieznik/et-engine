#include <et/geometry/rectplacer.h>
#include <et/gui/charactergenerator.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/imagewriter.h>
#include <et/timers/intervaltimer.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

using namespace et;
using namespace et::gui;

const int defaultTextureBindUnit = 7;
const int defaultTextureSize = 1024;

class et::gui::CharacterGeneratorPrivate
{
	public:
		CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size);
		~CharacterGeneratorPrivate();

		void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
			Texture texture, BinaryDataStorage& data);

		void renderCharacter(int value, bool bold, const vec2i& size, BinaryDataStorage&);

	public:
		size_t _size;		
		std::string _face;
		std::string _boldFace;

		RectPlacer placer;
		BinaryDataStorage _textureData;

		HDC commonDC;
		HFONT _font;
		HFONT _boldFont;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face, const std::string& boldFace, size_t size) : _rc(rc),
	_private(new CharacterGeneratorPrivate(face, boldFace, size)), _face(face), _boldFace(boldFace), _size(size)
{
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(defaultTextureSize), GL_RGBA, 
		GL_UNSIGNED_BYTE, BinaryDataStorage(4 * defaultTextureSize * defaultTextureSize, 0), face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool updateTexture)
{
	SIZE characterSize = { };
	wchar_t string[2] = { static_cast<wchar_t>(value), 0 };

	SelectObject(_private->commonDC, _private->_font);
	GetTextExtentPointW(_private->commonDC, string, 1, &characterSize);
	vec2i charSize(characterSize.cx, characterSize.cy);

	CharDescriptor desc(value);
	if (charSize.square() > 0)
	{
		rect textureRect;

		BinaryDataStorage data(charSize.square() * 4, 0);
		_private->placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(value, false, charSize, data);

		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = textureRect.origin() + vec2(1.0f);
		desc.size = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}

	_chars[value] = desc;
	return desc;
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool updateTexture)
{
	SIZE characterSize = { };
	wchar_t string[2] = { static_cast<wchar_t>(value), 0 };

	SelectObject(_private->commonDC, _private->_boldFont);
	GetTextExtentPointW(_private->commonDC, string, 1, &characterSize);

	vec2i charSize(characterSize.cx, characterSize.cy);

	CharDescriptor desc(value, CharParameter_Bold);
	if (charSize.square() > 0)
	{
		rect textureRect;

		BinaryDataStorage data(charSize.square() * 4, 0);
		_private->placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(value, true, charSize, data);

		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = textureRect.origin() + vec2(1.0f);
		desc.size = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}

	_boldChars[value] = desc;
	return desc;
}

/*
 * Private
 */

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size) : 
	_size(size), _face(face), _boldFace(boldFace), placer(vec2i(defaultTextureSize), true), _textureData(sqr(defaultTextureSize) * 4)
{
	commonDC = CreateCompatibleDC(nullptr);

	int pointsSize = -MulDiv(size, GetDeviceCaps(commonDC, LOGPIXELSY), 72);

	_font = CreateFont(pointsSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, face.c_str());

	_boldFont = CreateFont(pointsSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, boldFace.c_str());
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
	DeleteObject(_font);
	DeleteObject(_boldFont);
	DeleteDC(commonDC);
}

void CharacterGeneratorPrivate::updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
	Texture texture, BinaryDataStorage& data)
{
	vec2i dest(position.x, defaultTextureSize - position.y - size.y);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(int value, bool bold, const vec2i& size, BinaryDataStorage& data)
{
	RECT r = { 0, 0, size.x, size.y };
	wchar_t string[2] = { static_cast<wchar_t>(value), 0 };

	HDC dc = CreateCompatibleDC(nullptr);
	HBITMAP bitmap = CreateBitmap(size.x, size.y, 1, 32, nullptr);
	BITMAPINFO bitmapInfo = { };
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = size.x;
	bitmapInfo.bmiHeader.biHeight = size.y;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;

	SelectObject(dc, bitmap);
	SelectObject(dc, bold ? _boldFont : _font);
	SetTextColor(dc, RGB(255, 255, 255));
	SetBkMode(dc, TRANSPARENT);

	DrawTextW(dc, string, -1, &r, 0);
	GetDIBits(dc, bitmap, 0, size.y, data.binary(), &bitmapInfo, DIB_RGB_COLORS);

	unsigned int* ptr = reinterpret_cast<unsigned int*>(data.data());
	unsigned int* ptrEnd = reinterpret_cast<unsigned int*>(data.data() + data.dataSize());
	while (ptr != ptrEnd)
	{
		unsigned int& value = *ptr++;
		value |= 0x00ffffff | ((((value & 0x000000ff) + ((value & 0x0000ff00) >> 8) + ((value & 0x00ff0000) >> 16)) / 3) << 24);
	}

	DeleteObject(bitmap);
	DeleteDC(dc);
}

#endif // PLATFORM_WIN
