#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/AppKit.h>
#include <et/platform-apple/objc.h>
#include <et/geometry/rectplacer.h>
#include <et/gui/charactergenerator.h>
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
	size_t fontSize = 0;
	
	RectPlacer _placer;
	
	NSFont* font = nil;
	NSFont* boldFont = nil;
	NSColor* whiteColor = nil;

	CGColorSpaceRef colorSpace = nullptr;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size) : _rc(rc), _face(face), _size(size)
{
	ET_PIMPL_INIT(CharacterGenerator, face, boldFace, size)
	
#if !defined(ET_CONSOLE_APPLICATION)
	BinaryDataStorage emptyData(defaultTextureSize * defaultTextureSize * 4, 0);
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(defaultTextureSize),
		GL_RGBA, GL_UNSIGNED_BYTE, emptyData, face + "font");
#endif
}

CharacterGenerator::~CharacterGenerator()
{
	ET_PIMPL_FINALIZE(CharacterGenerator)
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool)
{
	CharDescriptor desc(value);
	
#if !defined(ET_CONSOLE_APPLICATION)
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
	
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square() * 4, 0);
		
		recti textureRect;
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = vector2ToFloat(textureRect.origin() + vec2i(1));
		desc.size = vector2ToFloat(textureRect.size() - vec2i(2));
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}

	_chars[value] = desc;
#endif
	
	return desc;
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool)
{
	CharDescriptor desc(value, CharParameter_Bold);
	
#if !defined(ET_CONSOLE_APPLICATION)
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
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square() * 4, 0);

		recti textureRect;
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = vector2ToFloat(textureRect.origin() + vec2i(1));
		desc.size = vector2ToFloat(textureRect.size() - vec2i(2));
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}
	
	_boldChars[value] = desc;
#endif
	
	return desc;
}

/*
 * Private
 */

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face,
	const std::string&, size_t size) : fontFace(face), fontSize(size),
	_placer(vec2i(defaultTextureSize), true)
{
#if !defined(ET_CONSOLE_APPLICATION)
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
#endif
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
#if !defined(ET_CONSOLE_APPLICATION)
	CGColorSpaceRelease(colorSpace);
	ET_OBJC_RELEASE(whiteColor);
    ET_OBJC_RELEASE(font);
	ET_OBJC_RELEASE(boldFont);
#endif
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
#if !defined(ET_CONSOLE_APPLICATION)
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
#endif
}

#endif // ET_PLATFORM_MAC
