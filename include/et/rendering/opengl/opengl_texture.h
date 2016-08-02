/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>

namespace et
{
	class OpenGLTexture : public Texture
	{
	public:
		ET_DECLARE_POINTER(OpenGLTexture);
		
	public:
		OpenGLTexture(const TextureDescription::Pointer&);
		~OpenGLTexture();

        void bind(uint32_t unit) override;
        void update(TextureDescription::Pointer desc) override;
        
		void setWrap(TextureWrap s, TextureWrap t, TextureWrap r = TextureWrap::ClampToEdge);
		void setFiltration(TextureFiltration minFiltration, TextureFiltration magFiltration);

		void setMaxLod(uint32_t value);
		void setAnisotropyLevel(float);

		void compareRefToTexture(bool enable, int32_t compareFunc);
		void generateMipMaps();

		vec2 getTexCoord(const vec2& ivec, TextureOrigin origin = TextureOrigin::TopLeft) const;

		void updateDataDirectly(const vec2i& size, const char* data, size_t dataSize);

		void updatePartialDataDirectly(const vec2i& offset, const vec2i& size,
			const char* data, size_t dataSize);

		uint32_t apiHandle() const { return _ah; }

	private:
		void generateTexture();
		void buildProperies();
		void build();
        void buildData(const char* ptr, size_t dataSize);

		uint32_t _ah = 0;
		void setAPIHandle(uint32_t ah) { _ah = ah; }

	private:
		vector3<TextureWrap> _wrap = vector3<TextureWrap>(TextureWrap::Repeat);
		vector2<TextureFiltration> _filtration = vector2<TextureFiltration>(TextureFiltration::Linear);
	};
}
