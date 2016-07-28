/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/rendering/opengl/opengl.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/rendercontext.h>

using namespace et;

static const int defaultBindingUnit = 7;

Texture::Texture(const TextureDescription::Pointer& desc, const std::string& id, bool deferred) :
	LoadableObject(id, desc->origin()), _desc(desc), _own(true)
{
#if (ET_OPENGLES)
	if (!(isPowerOfTwo(desc->size.x) && isPowerOfTwo(desc->size.y)))
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);
	
	const auto& caps = OpenGLCapabilities::instance();
	if ((_desc->internalformat == TextureFormat::R) && (caps.version() > OpenGLVersion::Version_2x))
		_desc->internalformat = TextureFormat::R8;
#endif
	
	if (deferred)
	{
		buildProperies();
	}
	else
	{
		generateTexture();
		build();
	}
}

Texture::Texture(uint32_t texture, const vec2i& size, const std::string& name) :
	LoadableObject(name), _desc(etCreateObject<TextureDescription>())
{
	if (glIsTexture(texture))
	{
		_desc->setOrigin(name);
		_desc->target = TextureTarget::Texture_2D;
		_desc->size = size;
		_desc->mipMapCount = 1;
		
		setAPIHandle(texture);
		buildProperies();
	}
}

Texture::~Texture()
{
	uint32_t texture = apiHandle();
	if (_own && (texture != 0) && glIsTexture(texture))
		glDeleteTextures(1, &texture);
}

void Texture::setWrap(TextureWrap s, TextureWrap t, TextureWrap r)
{
	bind(defaultBindingUnit);

	_wrap = vector3<TextureWrap>(s, t, r);

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
}

void Texture::setFiltration(TextureFiltration minFiltration,
	TextureFiltration magFiltration)
{
	bind(defaultBindingUnit);

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
}

#if defined(GL_TEXTURE_COMPARE_MODE) && defined(GL_TEXTURE_COMPARE_FUNC)

void Texture::compareRefToTexture(bool enable, int32_t compareFunc)
{
	bind(defaultBindingUnit);

	auto targetValue = textureTargetValue(_desc->target);
	
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
}

#else

void Texture::compareRefToTexture(bool, int32_t)
	{ ET_FAIL("GL_TEXTURE_COMPARE_MODE and GL_TEXTURE_COMPARE_FUNC are not defined."); }

#endif

void Texture::generateTexture()
{
	uint32_t texture = apiHandle();
	bool validTexture = glIsTexture(texture) != 0;
	checkOpenGLError("glIsTexture - %s", name().c_str());

	if (!validTexture)
	{
		glGenTextures(1, &texture);
		checkOpenGLError("glGenTextures - %s", name().c_str());
		setAPIHandle(texture);
	}
}

void Texture::buildData(const char* aDataPtr, size_t aDataSize)
{
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
	auto typeValue = dataFormatValue(_desc->type);

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
				etCompressedTexImage2D(targetValue, static_cast<int32_t>(level),
					internalFormatValue, t_mipSize.x, t_mipSize.y, 0, t_dataSize, ptr);
			}
			else
			{
				etTexImage2D(targetValue, static_cast<int32_t>(level), internalFormatValue,
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
					etCompressedTexImage2D(target, static_cast<int32_t>(level), internalFormatValue,
						t_mipSize.x, t_mipSize.y, 0, static_cast<GLsizei>(_desc->dataSizeForMipLevel(level)), ptr);
				}
				else
				{
					etTexImage2D(target, static_cast<int32_t>(level), internalFormatValue, t_mipSize.x,
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
}

void Texture::buildProperies()
{
	setOrigin(_desc->origin());
	
	_texel = vec2(1.0f / static_cast<float>(_desc->size.x), 1.0f / static_cast<float>(_desc->size.y) );
	_filtration.x = (_desc->mipMapCount > 1) ? TextureFiltration::LinearMipMapLinear : TextureFiltration::Linear;
	_filtration.y = TextureFiltration::Linear;
}

void Texture::bind(uint32_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(textureTargetValue(_desc->target), apiHandle());
}

void Texture::build()
{
	ET_ASSERT(_desc.valid());
	
	if (_desc->size.square() <= 0)
	{
		log::warning("Texture '%s' has invalid dimensions.", _desc->origin().c_str());
		return;
	}
	
	bind(defaultBindingUnit);
	buildProperies();

	if (_desc->target == TextureTarget::Texture_Rectangle)
	{
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);

		if (_filtration.x > TextureFiltration::Linear)
			_filtration.x = TextureFiltration::Linear;
	}

	setFiltration(_filtration.x, _filtration.y);
	setWrap(_wrap.x, _wrap.y, _wrap.z);

	buildData(_desc->data.constBinaryData(), _desc->data.dataSize());
	checkOpenGLError("buildData");

	if (_desc->mipMapCount > 1)
		setMaxLod(_desc->mipMapCount - 1);
	
	_desc->data.resize(0);
}

vec2 Texture::getTexCoord(const vec2& vec, TextureOrigin origin) const
{
	float ax = vec.x * _texel.x;
	float ay = vec.y * _texel.y;
	return vec2(ax, (origin == TextureOrigin::TopLeft) ? 1.0f - ay : ay);
}

void Texture::updateData(TextureDescription::Pointer desc)
{
	_desc = desc;
	generateTexture();
	build();
}

void Texture::updateDataDirectly(const vec2i& size, const char* data, size_t dataSize)
{
	if (apiHandle() == 0)
		generateTexture();

    _desc->size = size;

	bind(defaultBindingUnit);
	buildData(data, dataSize);
}

void Texture::updatePartialDataDirectly(const vec2i& offset, const vec2i& aSize, const char* data, size_t)
{
	ET_ASSERT((_desc->target == TextureTarget::Texture_2D) && !_desc->compressed);
	ET_ASSERT((offset.x >= 0) && (offset.y >= 0));
	ET_ASSERT((offset.x + aSize.x) < _desc->size.x);
	ET_ASSERT((offset.y + aSize.y) < _desc->size.y);
	
	if (apiHandle() == 0)
		generateTexture();
	
	bind(defaultBindingUnit);

	glPixelStorei(GL_UNPACK_ALIGNMENT, _desc->alignment);
	checkOpenGLError("glPixelStorei");

#if defined(GL_UNPACK_ROW_LENGTH)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, _desc->rowSize);
	checkOpenGLError("glPixelStorei");
#endif
	
	auto targetValue = textureTargetValue(_desc->target);
	auto typeValue = dataFormatValue(_desc->type);
	auto formatValue = textureFormatValue(_desc->format);
	
	glTexSubImage2D(targetValue, 0, offset.x, offset.y, aSize.x, aSize.y, formatValue, typeValue, data);
	
	checkOpenGLError("glTexSubImage2D(%s, 0, %d, %d, %d, %d, %s, %s, 0x%016x)",
		glTexTargetToString(targetValue).c_str(), offset.x, offset.y, aSize.x, aSize.y,
		glInternalFormatToString(formatValue).c_str(), glTypeToString(typeValue).c_str(), data);
}

void Texture::generateMipMaps()
{
	bind(defaultBindingUnit);
	glGenerateMipmap(textureTargetValue(_desc->target));
	checkOpenGLError("glGenerateMipmap");
}

void Texture::setMaxLod(uint32_t value)
{
#if defined(GL_TEXTURE_MAX_LEVEL)
	bind(defaultBindingUnit);
	glTexParameteri(textureTargetValue(_desc->target), GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(value));
	checkOpenGLError("Texture::setMaxLod(%u) - %s", static_cast<uint32_t>(value), name().c_str());
#endif
}

void Texture::setAnisotropyLevel(float value)
{
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
	bind(defaultBindingUnit);
	glTexParameterf(textureTargetValue(_desc->target), GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
	checkOpenGLError("Texture::setAnisotropyLevel(%f) - %s", value, name().c_str());
#endif
}
