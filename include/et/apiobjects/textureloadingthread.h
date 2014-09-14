/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <queue>
#include <et/threading/thread.h>
#include <et/threading/criticalsection.h>
#include <et/apiobjects/texture.h>

namespace et
{
	struct TextureLoadingRequest;
	typedef std::list<TextureLoadingRequest*> TextureLoadingRequestList;

	class TextureLoaderDelegate
	{
	public:
		virtual ~TextureLoaderDelegate();

		virtual void textureDidStartLoading(Texture t) = 0;
		virtual void textureDidLoad(Texture t) = 0;

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
		Texture texture;
		TextureLoaderDelegate* delegate;

		TextureLoadingRequest(const std::string& name, const Texture& tex, TextureLoaderDelegate* d);
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

		void addRequest(const std::string& fileName, Texture texture, TextureLoaderDelegate* delegate);

	private:
		ThreadResult main();
		TextureLoadingRequest* dequeRequest();

	private:
		TextureLoadingThreadDelegate* _delegate;
		TextureLoadingRequestQueue _requests;
		CriticalSection _requestsCriticalSection;
		
		volatile bool _running;
	};
}
