/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/rendering/opengl/opengl.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/opengl/opengl_texture.h>
#include <et/rendering/rendercontext.h>

using namespace et;

static const int defaultBindingUnit = 7;

OpenGLTexture::OpenGLTexture(const TextureDescription::Pointer& desc) :
	Texture(desc)
{
#if (ET_OPENGLES)
	if (!(isPowerOfTwo(desc->size.x) && isPowerOfTwo(desc->size.y)))
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);
	
	const auto& caps = OpenGLCapabilities::instance();
	if ((description()->internalformat == TextureFormat::R) && (caps.version() > OpenGLVersion::Version_2x))
		description()->internalformat = TextureFormat::R8;
#endif
	
    generateTexture();
    build();
}

OpenGLTexture::~OpenGLTexture()
{
	uint32_t texture = apiHandle();
	if ((texture != 0) && glIsTexture(texture))
		glDeleteTextures(1, &texture);
}

void OpenGLTexture::setWrap(TextureWrap s, TextureWrap t, TextureWrap r)
{
	bind(defaultBindingUnit);

	_wrap = vector3<TextureWrap>(s, t, r);

	auto targetValue = textureTargetValue(description()->target);
	
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

void OpenGLTexture::setFiltration(TextureFiltration minFiltration,
	TextureFiltration magFiltration)
{
	bind(defaultBindingUnit);

	_filtration = vector2<TextureFiltration>(minFiltration, magFiltration);

	if ((description()->mipMapCount < 2) && (minFiltration > TextureFiltration::Linear))
		_filtration.x = TextureFiltration::Linear;

	if (magFiltration > TextureFiltration::Linear)
		_filtration.y = TextureFiltration::Linear;

	auto targetValue = textureTargetValue(description()->target);
	
	glTexParameteri(targetValue, GL_TEXTURE_MIN_FILTER, textureFiltrationValue(_filtration.x));
	checkOpenGLError("glTexParameteri<GL_TEXTURE_MIN_FILTER> - %s", name().c_str());
	
	glTexParameteri(targetValue, GL_TEXTURE_MAG_FILTER, textureFiltrationValue(_filtration.y));
	checkOpenGLError("glTexParameteri<GL_TEXTURE_MAG_FILTER> - %s", name().c_str());
}

#if defined(GL_TEXTURE_COMPARE_MODE) && defined(GL_TEXTURE_COMPARE_FUNC)

void OpenGLTexture::compareRefToTexture(bool enable, int32_t compareFunc)
{
	bind(defaultBindingUnit);

	auto targetValue = textureTargetValue(description()->target);
	
	if (enable)
	{
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		checkOpenGLError("glTexParameteri(description()->target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)");
		
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_FUNC, compareFunc);
		checkOpenGLError("glTexParameteri(description()->target, GL_TEXTURE_COMPARE_FUNC, compareFunc)");
	}
	else
	{
		glTexParameteri(targetValue, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		checkOpenGLError("glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_NONE) - %s", name().c_str());
	}
}

#else

void OpenGLTexture::compareRefToTexture(bool, int32_t)
	{ ET_FAIL("GL_TEXTURE_COMPARE_MODE and GL_TEXTURE_COMPARE_FUNC are not defined."); }

#endif

void OpenGLTexture::generateTexture()
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

void OpenGLTexture::buildData(const char* aDataPtr, size_t aDataSize)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, description()->alignment);
	checkOpenGLError("glPixelStorei");

#if defined(GL_UNPACK_ROW_LENGTH)
	const auto& caps = OpenGLCapabilities::instance();
	if (!caps.isOpenGLES() || (caps.isOpenGLES() && caps.version() > OpenGLVersion::Version_2x))
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, description()->rowSize);
		checkOpenGLError("glPixelStorei");
	}
#endif
	
	auto targetValue = textureTargetValue(description()->target);
	auto internalFormatValue = textureFormatValue(description()->internalformat);
	auto formatValue = textureFormatValue(description()->format);
	auto typeValue = dataFormatValue(description()->type);

	if ((description()->target == TextureTarget::Texture_2D) || (description()->target == TextureTarget::Texture_Rectangle))
	{
		for (size_t level = 0; level < description()->mipMapCount; ++level)
		{
			vec2i t_mipSize = description()->sizeForMipLevel(level);
			GLsizei t_dataSize = static_cast<GLsizei>(description()->dataSizeForMipLevel(level));
			size_t t_offset = description()->dataOffsetForMipLevel(level, 0);
			
			const char* ptr = (aDataPtr && (t_offset < aDataSize)) ? &aDataPtr[t_offset] : 0;
			if (description()->compressed && ptr)
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
	else if (description()->target == TextureTarget::Texture_Cube)
	{
		uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		
		for (size_t layer = 0; layer < description()->layersCount; ++layer, ++target)
		{
			for (size_t level = 0; level < description()->mipMapCount; ++level)
			{
				vec2i t_mipSize = description()->sizeForMipLevel(level);
				size_t t_offset = description()->dataOffsetForMipLevel(level, layer);
				
				const char* ptr = (aDataPtr && (t_offset < aDataSize)) ? &aDataPtr[t_offset] : nullptr;
				if (description()->compressed && (ptr != nullptr))
				{
					etCompressedTexImage2D(target, static_cast<int32_t>(level), internalFormatValue,
						t_mipSize.x, t_mipSize.y, 0, static_cast<GLsizei>(description()->dataSizeForMipLevel(level)), ptr);
				}
				else
				{
					etTexImage2D(target, static_cast<int32_t>(level), internalFormatValue, t_mipSize.x,
						t_mipSize.y, 0, formatValue, typeValue, ptr);
				}
			}
		}
	}
	else if (description()->target == TextureTarget::Texture_2D_Array)
	{
		ET_ASSERT(!description()->compressed);
		ET_ASSERT(description()->mipMapCount == 1);

		etTexImage3D(targetValue, 0, internalFormatValue, description()->size.x, description()->size.y, description()->layersCount, 
			0, formatValue, typeValue, aDataPtr);
	}
	else
	{
		ET_FAIL_FMT("Unsupported texture target specified: %s", glTexTargetToString(targetValue).c_str());

	}
}

void OpenGLTexture::buildProperies()
{
	setOrigin(description()->origin());
	_filtration.x = (description()->mipMapCount > 1) ? TextureFiltration::LinearMipMapLinear : TextureFiltration::Linear;
	_filtration.y = TextureFiltration::Linear;
}

void OpenGLTexture::bind(uint32_t unit)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(textureTargetValue(description()->target), apiHandle());
}

void OpenGLTexture::build()
{
	ET_ASSERT(description().valid());
	
	if (description()->size.square() <= 0)
	{
		log::warning("Texture '%s' has invalid dimensions.", description()->origin().c_str());
		return;
	}
	
	bind(defaultBindingUnit);
	buildProperies();

	if (description()->target == TextureTarget::Texture_Rectangle)
	{
		_wrap = vector3<TextureWrap>(TextureWrap::ClampToEdge);

		if (_filtration.x > TextureFiltration::Linear)
			_filtration.x = TextureFiltration::Linear;
	}

	setFiltration(_filtration.x, _filtration.y);
	setWrap(_wrap.x, _wrap.y, _wrap.z);

	buildData(description()->data.constBinaryData(), description()->data.dataSize());
	checkOpenGLError("buildData");

	if (description()->mipMapCount > 1)
		setMaxLod(description()->mipMapCount - 1);
	
	description()->data.resize(0);
}

vec2 OpenGLTexture::getTexCoord(const vec2& vec, TextureOrigin origin) const
{
    vec2 tc = vec * texel();
	return vec2(tc.x, (origin == TextureOrigin::TopLeft) ? 1.0f - tc.y : tc.y);
}

void OpenGLTexture::update(TextureDescription::Pointer desc)
{
    setDescription(desc);
	generateTexture();
	build();
}

void OpenGLTexture::updateDataDirectly(const vec2i& size, const char* data, size_t dataSize)
{
	if (apiHandle() == 0)
		generateTexture();

    description()->size = size;

	bind(defaultBindingUnit);
	buildData(data, dataSize);
}

void OpenGLTexture::updatePartialDataDirectly(const vec2i& offset, const vec2i& aSize, const char* data, size_t)
{
	ET_ASSERT((description()->target == TextureTarget::Texture_2D) && !description()->compressed);
	ET_ASSERT((offset.x >= 0) && (offset.y >= 0));
	ET_ASSERT((offset.x + aSize.x) < description()->size.x);
	ET_ASSERT((offset.y + aSize.y) < description()->size.y);
	
	if (apiHandle() == 0)
		generateTexture();
	
	bind(defaultBindingUnit);

	glPixelStorei(GL_UNPACK_ALIGNMENT, description()->alignment);
	checkOpenGLError("glPixelStorei");

#if defined(GL_UNPACK_ROW_LENGTH)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, description()->rowSize);
	checkOpenGLError("glPixelStorei");
#endif
	
	auto targetValue = textureTargetValue(description()->target);
	auto typeValue = dataFormatValue(description()->type);
	auto formatValue = textureFormatValue(description()->format);
	
	glTexSubImage2D(targetValue, 0, offset.x, offset.y, aSize.x, aSize.y, formatValue, typeValue, data);
	
	checkOpenGLError("glTexSubImage2D(%s, 0, %d, %d, %d, %d, %s, %s, 0x%016x)",
		glTexTargetToString(targetValue).c_str(), offset.x, offset.y, aSize.x, aSize.y,
		glInternalFormatToString(formatValue).c_str(), glTypeToString(typeValue).c_str(), data);
}

void OpenGLTexture::generateMipMaps()
{
	bind(defaultBindingUnit);
	glGenerateMipmap(textureTargetValue(description()->target));
	checkOpenGLError("glGenerateMipmap");
}

void OpenGLTexture::setMaxLod(uint32_t value)
{
#if defined(GL_TEXTURE_MAX_LEVEL)
	bind(defaultBindingUnit);
	glTexParameteri(textureTargetValue(description()->target), GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(value));
	checkOpenGLError("OpenGLTexture::setMaxLod(%u) - %s", static_cast<uint32_t>(value), name().c_str());
#endif
}

void OpenGLTexture::setAnisotropyLevel(float value)
{
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
	bind(defaultBindingUnit);
	glTexParameterf(textureTargetValue(description()->target), GL_TEXTURE_MAX_ANISOTROPY_EXT, value);
	checkOpenGLError("OpenGLTexture::setAnisotropyLevel(%f) - %s", value, name().c_str());
#endif
}
