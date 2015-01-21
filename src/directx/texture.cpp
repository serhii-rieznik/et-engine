/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

#include <et/core/tools.h>

using namespace et;

#if !defined(ET_CONSOLE_APPLICATION)
	static const int defaultBindingUnit = 7;
#endif

Texture::Texture(RenderContext* rc, const TextureDescription::Pointer& desc, const std::string& id,
	bool deferred) : APIObject(id, desc->origin()), _desc(desc), _own(true)
{
#if defined(ET_CONSOLE_APPLICATION)

	ET_FAIL("Attempt to create Texture in console application.")

#else

#	if (ET_OPENGLES)
	if (!(isPowerOfTwo(desc->size.x) && isPowerOfTwo(desc->size.y)))
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);
#	endif
	
	if (deferred)
	{
		buildProperies();
	}
	else
	{
		generateTexture(rc);
		build(rc);
	}
	
#endif
}

Texture::Texture(RenderContext*, uint32_t texture, const vec2i& size, const std::string& name) :
	APIObject(name), _own(false), _desc(sharedObjectFactory().createObject<TextureDescription>())
{
#if !defined(ET_CONSOLE_APPLICATION)
	setAPIHandle(0);
	buildProperies();
#endif
}

Texture::~Texture()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Texture::setWrap(RenderContext* rc, TextureWrap s, TextureWrap t, TextureWrap r)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_wrap = vector3<TextureWrap>(s, t, r);
#endif
}

void Texture::setFiltration(RenderContext* rc, TextureFiltration minFiltration,
	TextureFiltration magFiltration)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Texture::compareRefToTexture(RenderContext* rc, bool enable, int32_t compareFunc)
{
}

void Texture::generateTexture(RenderContext*)
{
#if !defined(ET_CONSOLE_APPLICATION)
	setAPIHandle(0);
#endif
}

void Texture::buildData(const char* aDataPtr, size_t aDataSize)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Texture::buildProperies()
{
	setOrigin(_desc->origin());
	
	_texel = vec2(1.0f / static_cast<float>(_desc->size.x), 1.0f / static_cast<float>(_desc->size.y) );
	_filtration.x = (_desc->mipMapCount > 1) ? TextureFiltration::LinearMipMapLinear : TextureFiltration::Linear;
	_filtration.y = TextureFiltration::Linear;
}

void Texture::build(RenderContext* rc)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(_desc.valid());
	
	if (_desc->size.square() <= 0)
	{
		log::warning("Texture '%s' has invalid dimensions.", _desc->origin().c_str());
		return;
	}
	
	buildProperies();

	rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target, true);

	setFiltration(rc, _filtration.x, _filtration.y);
	setWrap(rc, _wrap.x, _wrap.y, _wrap.z);

	if (_desc->mipMapCount > 1)
		setMaxLod(rc, _desc->mipMapCount - 1);

    buildData(_desc->data.constBinaryData(), _desc->data.dataSize());

	_desc->data.resize(0);
#endif
}

vec2 Texture::getTexCoord(const vec2& vec, TextureOrigin origin) const
{
	float ax = vec.x * _texel.x;
	float ay = vec.y * _texel.y;
	return vec2(ax, (origin == TextureOrigin::TopLeft) ? 1.0f - ay : ay);
}

void Texture::updateData(RenderContext* rc, TextureDescription::Pointer desc)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_desc = desc;
	generateTexture(rc);
	build(rc);
#endif
}

void Texture::updateDataDirectly(RenderContext* rc, const vec2i& size, const char* data, size_t dataSize)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (apiHandleInvalid())
		generateTexture(rc);

    _desc->size = size;
    rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	buildData(data, dataSize);
#endif
}

void Texture::updatePartialDataDirectly(RenderContext* rc, const vec2i& offset,
	const vec2i& aSize, const char* data, size_t)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT((_desc->target == TextureTarget::Texture_2D) && !_desc->compressed);
	ET_ASSERT((offset.x >= 0) && (offset.y >= 0));
	ET_ASSERT((offset.x + aSize.x) < _desc->size.x);
	ET_ASSERT((offset.y + aSize.y) < _desc->size.y);
	
	if (apiHandleInvalid())
		generateTexture(rc);
	
#endif
}

void Texture::generateMipMaps(RenderContext* rc)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Texture::setMaxLod(RenderContext* rc, size_t value)
{
#if defined(GL_TEXTURE_MAX_LEVEL) && !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Texture::setAnisotropyLevel(RenderContext* rc, float value)
{
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && !defined(ET_CONSOLE_APPLICATION)
#endif
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
