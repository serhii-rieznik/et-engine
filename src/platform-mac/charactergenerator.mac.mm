#include <AppKit/AppKit.h>
#include <et/platform-apple/objc.h>
#include <et/geometry/rectplacer.h>
#include <et/gui/charactergenerator.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/imagewriter.h>

using namespace et;
using namespace et::gui;

const int defaultTextureSize = 1024;

class et::gui::CharacterGeneratorPrivate
{
public:
	CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size);
	~CharacterGeneratorPrivate();
	
	void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
		Texture texture, BinaryDataStorage& data);
	
	void renderCharacter(NSAttributedString* value, const vec2i& size,
		NSFont* font, BinaryDataStorage& data);
	
public:
	std::string fontFace;
	size_t fontSize;
	
	RectPlacer _placer;
	
	NSFont* font;
	NSFont* boldFont;
	NSColor* whiteColor;

	CGColorSpaceRef colorSpace;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size) : _rc(rc),
	_private(new CharacterGeneratorPrivate(face, boldFace, size)), _face(face), _size(size)
{
	BinaryDataStorage emptyData(defaultTextureSize * defaultTextureSize * 4, 0);
	
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(defaultTextureSize),
												GL_RGBA, GL_UNSIGNED_BYTE, emptyData, face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };
	
	NSString* wString = ET_OBJC_AUTORELEASE([[NSString alloc] initWithBytesNoCopy:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding freeWhenDone:NO]);

	NSMutableAttributedString* attrString =
		ET_OBJC_AUTORELEASE([[NSMutableAttributedString alloc] initWithString:wString]);

	NSRange wholeString = NSMakeRange(0, [attrString length]);
	[attrString addAttribute:NSFontAttributeName value:_private->font range:wholeString];
	[attrString addAttribute:NSForegroundColorAttributeName value:_private->whiteColor range:wholeString];

	NSSize characterSize = [attrString size];
	vec2i charSize = vec2i(static_cast<int>(characterSize.width),
        static_cast<int>(characterSize.height));
	
	CharDescriptor desc(value);
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square() * 4, 0);
		
		rect textureRect;
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
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

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };

	NSString* wString = ET_OBJC_AUTORELEASE([[NSString alloc] initWithBytesNoCopy:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding freeWhenDone:NO]);

	NSMutableAttributedString* attrString =
		ET_OBJC_AUTORELEASE([[NSMutableAttributedString alloc] initWithString:wString]);

	NSRange wholeString = NSMakeRange(0, [attrString length]);
	[attrString addAttribute:NSFontAttributeName value:_private->boldFont range:wholeString];
	[attrString addAttribute:NSForegroundColorAttributeName value:_private->whiteColor range:wholeString];

	NSSize characterSize = [attrString size];
	vec2i charSize = vec2i(static_cast<int>(characterSize.width),
        static_cast<int>(characterSize.height));
	
	CharDescriptor desc(value, CharParameter_Bold);
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square() * 4, 0);

		rect textureRect;
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
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

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face,
	const std::string&, size_t size) : fontFace(face), fontSize(size),
	_placer(vec2i(defaultTextureSize), true)
{
    NSString* cFace = [NSString stringWithCString:face.c_str() encoding:NSUTF8StringEncoding];
	
	font = ET_OBJC_RETAIN([[NSFontManager sharedFontManager] fontWithFamily:cFace traits:0 weight:0 size:size]);
		
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		font = ET_OBJC_RETAIN([[NSFontManager sharedFontManager]
			fontWithFamily:@"Arial" traits:0 weight:0 size:size]);
	}
	ET_ASSERT(font);
	
	boldFont = ET_OBJC_RETAIN([[NSFontManager sharedFontManager] fontWithFamily:cFace traits:NSBoldFontMask
		weight:0 size:size]);
	
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		boldFont = ET_OBJC_RETAIN([[NSFontManager sharedFontManager]
			fontWithFamily:@"Arial" traits:0 weight:0 size:size]);
	}
	ET_ASSERT(boldFont);

	whiteColor = ET_OBJC_RETAIN([NSColor whiteColor]);
			
	colorSpace = CGColorSpaceCreateDeviceRGB();
	ET_ASSERT(colorSpace);
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
	CGColorSpaceRelease(colorSpace);
	ET_OBJC_RELEASE(whiteColor);
    ET_OBJC_RELEASE(font);
	ET_OBJC_RELEASE(boldFont);
}

void CharacterGeneratorPrivate::updateTexture(RenderContext* rc, const vec2i& position,
	const vec2i& size, Texture texture, BinaryDataStorage& data)
{
	vec2i dest(position.x, defaultTextureSize - position.y - size.y);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(NSAttributedString* value,
	const vec2i& size, NSFont*, BinaryDataStorage& data)
{
	CGContextRef context = CGBitmapContextCreateWithData(data.data(), static_cast<size_t>(size.x),
		static_cast<size_t>(size.y), 8, 4 * static_cast<size_t>(size.x), colorSpace,
		kCGImageAlphaPremultipliedLast, nil, nil);
	ET_ASSERT(context);

	NSGraphicsContext* aContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
	[NSGraphicsContext setCurrentContext:aContext];

	[value drawAtPoint:NSMakePoint(0.0, 0.0)];
	
	[aContext flushGraphics];
	CGContextRelease(context);
	
	unsigned int* ptr = reinterpret_cast<unsigned int*>(data.data());
	unsigned int* endPtr = ptr + data.dataSize() / 4;
	while (ptr != endPtr)
		*ptr++ |= 0x00FFFFFF;
}
