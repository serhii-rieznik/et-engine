/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/texturedescription.h>
#include <et/opengl/opengl.h>

namespace et
{
	class RenderContext;

	class TextureData : public LoadableObject
	{
	public:
		TextureData(RenderContext* rc, const TextureDescription::Pointer&, const std::string&, bool deferred);
		TextureData(RenderContext* rc, uint32_t texture, const vec2i& size, const std::string& name);
		~TextureData();

		void setWrap(RenderContext* rc, TextureWrap s, TextureWrap t,
			TextureWrap r = TextureWrap_ClampToEdge);

		void setFiltration(RenderContext* rc, TextureFiltration minFiltration,
			TextureFiltration magFiltration);

		void setMaxLod(RenderContext* rc, size_t value);

		void compareRefToTexture(RenderContext* rc, bool enable, int32_t compareFunc);
		void generateMipMaps(RenderContext* rc);

		vec2 getTexCoord(const vec2& ivec, TextureOrigin origin = TextureOrigin_TopLeft) const;

		void updateData(RenderContext* rc, TextureDescription::Pointer desc);
		void updateDataDirectly(RenderContext* rc, const vec2i& size, const char* data, size_t dataSize);

		void updatePartialDataDirectly(RenderContext* rc, const vec2i& offset, const vec2i& size,
			const char* data, size_t dataSize);

		uint32_t glID() const
			{ return _glID; }

		int32_t internalFormat() const
			{ return _desc->internalformat; }

		uint32_t format() const
			{ return _desc->format; }

		uint32_t dataType() const
			{ return _desc->type; }

		uint32_t target() const
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

	private:
		void generateTexture(RenderContext* rc);
		void build(RenderContext* rc);
        void buildData(const char* ptr, size_t dataSize);

	private:
		uint32_t _glID;
		TextureDescription::Pointer _desc;
		vector3<TextureWrap> _wrap;
		vector2<TextureFiltration> _filtration;
		vec2 _texel;
		bool _own = false;
	};

	typedef IntrusivePtr<TextureData> Texture;
}
