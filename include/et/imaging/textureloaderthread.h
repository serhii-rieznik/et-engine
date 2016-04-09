/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <queue>
#include <et/core/thread.h>
#include <et/core/criticalsection.h>
#include <et/rendering/texture.h>

namespace et
{
	struct TextureLoadingRequest;
	typedef std::list<TextureLoadingRequest*> TextureLoadingRequestList;

	class TextureLoaderDelegate
	{
	public:
		virtual ~TextureLoaderDelegate();

		virtual void textureDidStartLoading(Texture::Pointer t) = 0;
		virtual void textureDidLoad(Texture::Pointer t) = 0;

	private:
		friend struct TextureLoadingRequest;

		void removeTextureLoadingRequest(TextureLoadingRequest* req)
			{ CriticalSectionScope lock(_csRequest); _requests.remove(req); }

		void addTextureLoadingRequest(TextureLoadingRequest* req)
			{ CriticalSectionScope lock(_csRequest); _requests.push_back(req); }

	private:
		CriticalSection _csRequest;
		TextureLoadingRequestList _requests;
	};

	struct TextureLoadingRequest
	{
		std::string fileName;
		TextureDescription::Pointer textureDescription;
		Texture::Pointer texture;
		TextureLoaderDelegate* delegate = nullptr;

		TextureLoadingRequest(const std::string& name, const Texture::Pointer& tex, TextureLoaderDelegate* d);
		~TextureLoadingRequest();
		
		void discardDelegate();
	};
	
	typedef std::queue<TextureLoadingRequest*> TextureLoadingRequestQueue;

	class TextureLoadingThread;
	class TextureLoadingThreadDelegate
	{
	public:
		virtual ~TextureLoadingThreadDelegate() { }
		virtual void textureLoadingThreadDidLoadTextureData(TextureLoadingRequest* request) = 0;
	};

	class TextureLoadingThread : public Thread
	{
	public:
		TextureLoadingThread(TextureLoadingThreadDelegate* delegate);
		~TextureLoadingThread();

		void addRequest(const std::string& fileName, Texture::Pointer texture, TextureLoaderDelegate* delegate);

	private:
		uint64_t main();
		TextureLoadingRequest* dequeRequest();

	private:
		TextureLoadingThreadDelegate* _delegate;
		TextureLoadingRequestQueue _requests;
		CriticalSection _requestsCriticalSection;
	};
}
