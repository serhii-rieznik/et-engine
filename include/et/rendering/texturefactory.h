/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/rendering/texture.h>
#include <et/rendering/apiobject.h>
#include <et/imaging/textureloaderthread.h>

namespace et
{
	class TextureFactoryPrivate;
	class TextureFactory : public APIObjectFactory, public TextureLoadingThreadDelegate
	{
	public:
		ET_DECLARE_POINTER(TextureFactory)
		
	public:
		TextureFactory(RenderContext*);
		
		~TextureFactory();
		
		Texture::Pointer loadTexture(const std::string& file, ObjectsCache& cache, bool async = false,
			TextureLoaderDelegate* delegate = nullptr);

		Texture::Pointer loadTexturesToCubemap(const std::string& posx, const std::string& negx,
			const std::string& posy, const std::string& negy, const std::string& posz,
			const std::string& negz, ObjectsCache& cache);

		Texture::Pointer genNoiseTexture(const vec2i& size, bool normalize, const std::string& aName);
		
		Texture::Pointer genCubeTexture(TextureFormat internalformat, uint32_t size, TextureFormat format,
			DataFormat type, const std::string& aName);

		Texture::Pointer genTexture2DArray(const vec3i& size, TextureTarget textureTarget, 
			TextureFormat internalformat, TextureFormat format, DataFormat type, const BinaryDataStorage& data,
			const std::string& aName);
		
		Texture::Pointer genTexture(TextureDescription::Pointer desc);
		
		Texture::Pointer genTexture(TextureTarget target, TextureFormat internalformat, const vec2i& size,
			TextureFormat format, DataFormat type, const BinaryDataStorage& data, const std::string& aName);
		
		Texture::Pointer createTextureWrapper(uint32_t texture, const vec2i& size, const std::string& aName);

		ObjectLoader::Pointer objectLoader();
		
		const StringList& supportedTextureExtensions() const;

		std::string resolveTextureName(const std::string&) const;
		
		ET_DECLARE_EVENT1(textureDidStartLoading, Texture::Pointer)
		ET_DECLARE_EVENT1(textureDidLoad, Texture::Pointer)

	private:
		ET_DENY_COPY(TextureFactory)
		
		friend class RenderContext;
		friend class TextureFactoryPrivate;
		
		void reloadObject(LoadableObject::Pointer, ObjectsCache&);
		void textureLoadingThreadDidLoadTextureData(TextureLoadingRequest* request);
		
	private:
		UniquePtr<TextureLoadingThread> _loadingThread;
		
		ET_DECLARE_PIMPL(TextureFactory, 64)

		CriticalSection _csTextureLoading;
		ObjectLoader::Pointer _loader;
	};

}
