/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/flags.h>
#include <et/rendering/rendering.h>

namespace et
{
	class RenderingCapabilities : public Singleton<RenderingCapabilities>
	{ 
	public:
		uint32_t maxTextureSize() const
			{ return _maxTextureSize;}
		
		uint32_t maxCubemapTextureSize() const
			{ return _maxCubemapTextureSize;}
		
		uint32_t maxSamples() const
			{ return _maxSamples; }
		
		float maxAnisotropyLevel() const
			{ return _maxAnisotropyLevel; }
		
		bool supportTextureFormat(TextureFormat fmt) const
			{ return (_textureFormatSupport.count(fmt) > 0) && (_textureFormatSupport.at(fmt) != 0); }
		
		void checkCaps();

	private:
		ET_SINGLETON_CONSTRUCTORS(RenderingCapabilities)
		
	private:
		uint32_t _maxTextureSize = 0;
		uint32_t _maxCubemapTextureSize = 0;
		uint32_t _maxSamples = 0;
		float _maxAnisotropyLevel = 0.0f;

		std::map<TextureFormat, uint32_t> _textureFormatSupport;
	};
}
