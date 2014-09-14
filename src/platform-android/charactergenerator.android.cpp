#include <et/rendering/rendercontext.h>
#include <et/geometry/rectplacer.h>
#include <et/gui/charactergenerator.h>
#include <et/imaging/imagewriter.h>

#if (ET_PLATFORM_ANDROID)

using namespace et;
using namespace et::gui;

const int defaultTextureSize = 1024;

class et::gui::CharacterGeneratorPrivate
{
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size) : _rc(rc), _private(new CharacterGeneratorPrivate),
	_face(face), _size(size)
{
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(defaultTextureSize),
		GL_RGBA, GL_UNSIGNED_BYTE, BinaryDataStorage(), face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool updateTexture)
{
	CharDescriptor desc(value);
	_chars[value] = desc;
	return desc;
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool updateTexture)
{
	CharDescriptor desc(value, CharParameter_Bold);
	_boldChars[value] = desc;
	return desc;
}

#endif // ET_PLATFORM_ANDROID
