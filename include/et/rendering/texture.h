/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/imaging/texturedescription.h>

namespace et
{
	class Texture : public LoadableObject
	{
	public:
		ET_DECLARE_POINTER(Texture);
		
	public:
		Texture(const TextureDescription::Pointer&, const std::string&, bool deferred);
		Texture(uint32_t texture, const vec2i& size, const std::string& name);
		~Texture();

		void bind(uint32_t unit) const;
		void setWrap(TextureWrap s, TextureWrap t, TextureWrap r = TextureWrap::ClampToEdge);
		void setFiltration(TextureFiltration minFiltration, TextureFiltration magFiltration);

		void setMaxLod(uint32_t value);
		void setAnisotropyLevel(float);

		void compareRefToTexture(bool enable, int32_t compareFunc);
		void generateMipMaps();

		vec2 getTexCoord(const vec2& ivec, TextureOrigin origin = TextureOrigin::TopLeft) const;

		void updateData(TextureDescription::Pointer desc);
		void updateDataDirectly(const vec2i& size, const char* data, size_t dataSize);

		void updatePartialDataDirectly(const vec2i& offset, const vec2i& size,
			const char* data, size_t dataSize);

		TextureFormat internalFormat() const
			{ return _desc->internalformat; }

		TextureFormat format() const
			{ return _desc->format; }

		DataFormat dataType() const
			{ return _desc->type; }

		TextureTarget target() const
			{ return _desc->target; }

		int width() const
			{ return _desc->size.x; }

		int height() const
			{ return _desc->size.y; }

		const vec2i& size() const
			{ return _desc->size; }

		vec2 sizeFloat() const
			{ return vec2(static_cast<float>(_desc->size.x), static_cast<float>(_desc->size.y)); }

		const vec2& texel() const
			{ return _texel; }
		
		const TextureDescription::Pointer description() const
			{ return _desc; }

		uint32_t apiHandle() const { return _ah; }

	private:
		void generateTexture();
		void buildProperies();
		void build();
        void buildData(const char* ptr, size_t dataSize);

		uint32_t _ah = 0;
		void setAPIHandle(uint32_t ah) { _ah = ah; }

	private:
		TextureDescription::Pointer _desc;
		vector3<TextureWrap> _wrap = vector3<TextureWrap>(TextureWrap::Repeat);
		vector2<TextureFiltration> _filtration = vector2<TextureFiltration>(TextureFiltration::Linear);
		vec2 _texel = vec2(0.0f);
		bool _own = false;
	};
}
