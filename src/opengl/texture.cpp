/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>

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
	
	const auto& caps = OpenGLCapabilities::instance();

	if ((_desc->internalformat == TextureFormat::R) && (caps.version() > OpenGLVersion::Version_2x))
		_desc->internalformat = TextureFormat::R8;
	
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
	APIObject(name), _own(false), _desc(etCreateObject<TextureDescription>())
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (glIsTexture(texture))
	{
		_desc->setOrigin(name);
		_desc->target = TextureTarget::Texture_2D;
		_desc->size = size;
		_desc->mipMapCount = 1;
		
		setAPIHandle(texture);
		buildProperies();
	}
#endif
}

Texture::~Texture()
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t texture = static_cast<uint32_t>(apiHandle());
	if (_own && (texture != 0) && glIsTexture(texture))
		glDeleteTextures(1, &texture);
#endif
}

void Texture::setWrap(RenderContext* rc, TextureWrap s, TextureWrap t, TextureWrap r)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_wrap = vector3<TextureWrap>(s, t, r);

	rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	
	auto targetValue = textureTargetValue(_desc->target);
	
	glTexParameteri(targetValue, GL_TEXTURE_WRAP_S, textureWrapValue(_wrap.x));
	checkOpenGLError("glTexParameteri<WRAP_S> - %s", name().c_str());
	
	glTexParameteri(targetValue, GL_TEXTURE_WRAP_T, textureWrapValue(_wrap.y));
	checkOpenGLError("glTexParameteri<WRAP_T> - %s", name().c_str());
	
#	if defined(GL_TEXTURE_WRAP_R)
#		if (ET_OPENGLES)
			if (OpenGLCapabilities::instance().version() > OpenGLVersion::Version_2x)
#		endif
	{
		glTexParameteri(targetValue, GL_TEXTURE_WRAP_R, textureWrapValue(_wrap.z));
		checkOpenGLError("glTexParameteri<WRAP_R> - %s", name().c_str());
	}
#	endif
#endif
}

void Texture::setFiltration(RenderContext* rc, TextureFiltration minFiltration,
	TextureFiltration magFiltration)
{
#if !defined(ET_CONSOLE_APPLICATION)
	rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);

	_filtration = vector2<TextureFiltration>(minFiltration, magFiltration);

	if ((_desc->mipMapCount < 2) && (minFiltration > TextureFiltration::Linear))
		_filtration.x = TextureFiltration::Linear;

	if (magFiltration > TextureFiltration::Linear)
		_filtration.y = TextureFiltration::Linear;

	auto targetValue = textureTargetValue(_desc->target);
	
	glTexParameteri(targetValue, GL_TEXTURE_MIN_FILTER, textureFiltrationValue(_filtration.x));
	checkOpenGLError("glTexParameteri<GL_TEXTURE_MIN_FILTER> - %s", name().c_str());
	
	glTexParameteri(targetValue, GL_TEXTURE_MAG_FILTER, textureFiltrationValue(_filtration.y));
	checkOpenGLError("glTexParameteri<GL_TEXTURE_MAG_FILTER> - %s", name().c_str());
#endif
}

#if defined(GL_TEXTURE_COMPARE_MODE) && defined(GL_TEXTURE_COMPARE_FUNC)
void Texture::compareRefToTexture(RenderContext* rc, bool enable, int32_t compareFunc)
{
#if !defined(ET_CONSOLE_APPLICATION)
	auto targetValue = textureTargetValue(_desc->target);
	
	rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	if (enable)
	{
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		checkOpenGLError("glTexParameteri(_desc->target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)");
		
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_FUNC, compareFunc);
		checkOpenGLError("glTexParameteri(_desc->target, GL_TEXTURE_COMPARE_FUNC, compareFunc)");
	}
	else
	{
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		checkOpenGLError("glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_NONE) - %s", name().c_str());
	}
#endif
}
#else
void Texture::compareRefToTexture(RenderContext*, bool, int32_t)
	{ ET_FAIL("GL_TEXTURE_COMPARE_MODE and GL_TEXTURE_COMPARE_FUNC are not defined."); }
#endif

void Texture::generateTexture(RenderContext*)
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t texture = static_cast<uint32_t>(apiHandle());
	bool validTexture = glIsTexture(texture) != 0;
	checkOpenGLError("glIsTexture - %s", name().c_str());

	if (!validTexture)
	{
		glGenTextures(1, &texture);
		checkOpenGLError("glGenTextures - %s", name().c_str());
		setAPIHandle(texture);
	}
#endif
}

void Texture::buildData(const char* aDataPtr, size_t aDataSize)
{
#if !defined(ET_CONSOLE_APPLICATION)

	glPixelStorei(GL_UNPACK_ALIGNMENT, _desc->alignment);
	checkOpenGLError("glPixelStorei");

#if defined(GL_UNPACK_ROW_LENGTH)
	const auto& caps = OpenGLCapabilities::instance();
	if (!caps.isOpenGLES() || (caps.isOpenGLES() && caps.version() > OpenGLVersion::Version_2x))
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, _desc->rowSize);
		checkOpenGLError("glPixelStorei");
	}
#endif
	
	auto targetValue = textureTargetValue(_desc->target);
	auto internalFormatValue = textureFormatValue(_desc->internalformat);
	auto formatValue = textureFormatValue(_desc->format);
	auto typeValue = dataTypeValue(_desc->type);

	if ((_desc->target == TextureTarget::Texture_2D) || (_desc->target == TextureTarget::Texture_Rectangle))
	{
		for (size_t level = 0; level < _desc->mipMapCount; ++level)
		{
			vec2i t_mipSize = _desc->sizeForMipLevel(level);
			GLsizei t_dataSize = static_cast<GLsizei>(_desc->dataSizeForMipLevel(level));
			size_t t_offset = _desc->dataOffsetForMipLevel(level, 0);
			
			const char* ptr = (aDataPtr && (t_offset < aDataSize)) ? &aDataPtr[t_offset] : 0;
			if (_desc->compressed && ptr)
			{
				etCompressedTexImage2D(targetValue, static_cast<int>(level),
					internalFormatValue, t_mipSize.x, t_mipSize.y, 0, t_dataSize, ptr);
			}
			else
			{
				etTexImage2D(targetValue, static_cast<int>(level), internalFormatValue,
					t_mipSize.x, t_mipSize.y, 0, formatValue, typeValue, ptr);
			}
		}
	}
	else if (_desc->target == TextureTarget::Texture_Cube)
	{
		uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		
		for (size_t layer = 0; layer < _desc->layersCount; ++layer, ++target)
		{
			for (size_t level = 0; level < _desc->mipMapCount; ++level)
			{
				vec2i t_mipSize = _desc->sizeForMipLevel(level);
				size_t t_offset = _desc->dataOffsetForMipLevel(level, layer);
				
				const char* ptr = (aDataPtr && (t_offset < aDataSize)) ? &aDataPtr[t_offset] : nullptr;
				if (_desc->compressed && (ptr != nullptr))
				{
					etCompressedTexImage2D(target, static_cast<int>(level), internalFormatValue,
						t_mipSize.x, t_mipSize.y, 0, static_cast<GLsizei>(_desc->dataSizeForMipLevel(level)), ptr);
				}
				else
				{
					etTexImage2D(target, static_cast<int>(level), internalFormatValue, t_mipSize.x,
						t_mipSize.y, 0, formatValue, typeValue, ptr);
				}
			}
		}
	}
	else if (_desc->target == TextureTarget::Texture_2D_Array)
	{
		ET_ASSERT(!_desc->compressed);
		ET_ASSERT(_desc->mipMapCount == 1);

		etTexImage3D(targetValue, 0, internalFormatValue, _desc->size.x, _desc->size.y, _desc->layersCount, 
			0, formatValue, typeValue, aDataPtr);
	}
	else
	{
		ET_FAIL_FMT("Unsupported texture target specified: %s", glTexTargetToString(targetValue).c_str());

	}
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

	if (_desc->target == TextureTarget::Texture_Rectangle)
	{
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);

		if (_filtration.x > TextureFiltration::Linear)
			_filtration.x = TextureFiltration::Linear;
	}

	setFiltration(rc, _filtration.x, _filtration.y);
	setWrap(rc, _wrap.x, _wrap.y, _wrap.z);

	buildData(_desc->data.constBinaryData(), _desc->data.dataSize());
	checkOpenGLError("buildData");

	if (_desc->mipMapCount > 1)
		setMaxLod(rc, _desc->mipMapCount - 1);
	
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
	
    rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, _desc->alignment);
	checkOpenGLError("glPixelStorei");

#if defined(GL_UNPACK_ROW_LENGTH)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, _desc->rowSize);
	checkOpenGLError("glPixelStorei");
#endif
	
	auto targetValue = textureTargetValue(_desc->target);
	auto typeValue = dataTypeValue(_desc->type);
	auto formatValue = textureFormatValue(_desc->format);
	
	glTexSubImage2D(targetValue, 0, offset.x, offset.y, aSize.x, aSize.y, formatValue, typeValue, data);
	
	checkOpenGLError("glTexSubImage2D(%s, 0, %d, %d, %d, %d, %s, %s, 0x%016x)",
		glTexTargetToString(targetValue).c_str(), offset.x, offset.y, aSize.x, aSize.y,
		glInternalFormatToString(formatValue).c_str(), glTypeToString(typeValue).c_str(), data);
#endif
}

void Texture::generateMipMaps(RenderContext* rc)
{
#if !defined(ET_CONSOLE_APPLICATION)
    rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	glGenerateMipmap(textureTargetValue(_desc->target));
	checkOpenGLError("glGenerateMipmap");
#endif
}

void Texture::setMaxLod(RenderContext* rc, size_t value)
{
#if defined(GL_TEXTURE_MAX_LEVEL) && !defined(ET_CONSOLE_APPLICATION)
    rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	glTexParameteri(textureTargetValue(_desc->target), GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(value));
	checkOpenGLError("Texture::setMaxLod(%lu) - %s", static_cast<unsigned long>(value), name().c_str());
#endif
}

void Texture::setAnisotropyLevel(RenderContext* rc, float value)
{
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && !defined(ET_CONSOLE_APPLICATION)
	rc->renderState().bindTexture(defaultBindingUnit, static_cast<uint32_t>(apiHandle()), _desc->target);
	glTexParameterf(textureTargetValue(_desc->target), GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
	checkOpenGLError("Texture::setAnisotropyLevel(%f) - %s", value, name().c_str());
#endif
}
