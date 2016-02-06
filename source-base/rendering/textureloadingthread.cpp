/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/app/invocation.h>
#include <et/imaging/textureloader.h>
#include <et/imaging/textureloaderthread.h>

using namespace et;

TextureLoaderDelegate::~TextureLoaderDelegate()
{
	for (auto i : _requests)
		i->discardDelegate();
}

TextureLoadingRequest::TextureLoadingRequest(const std::string& name, const Texture::Pointer& tex, TextureLoaderDelegate* d) :
	fileName(name), textureDescription(etCreateObject<TextureDescription>()), texture(tex), delegate(d)
{
	if (delegate)
		delegate->addTextureLoadingRequest(this);
}

TextureLoadingRequest::~TextureLoadingRequest()
{
	if (delegate)
		delegate->removeTextureLoadingRequest(this);
}

void TextureLoadingRequest::discardDelegate()
{
	delegate = nullptr;
}

TextureLoadingThread::TextureLoadingThread(TextureLoadingThreadDelegate* delegate) :
	Thread(false), _delegate(delegate) { }

TextureLoadingThread::~TextureLoadingThread()
{
	stop();

	CriticalSectionScope lock(_requestsCriticalSection);
	while (_requests.size())
	{
		etDestroyObject(_requests.front());
		_requests.pop();
	}
}

TextureLoadingRequest* TextureLoadingThread::dequeRequest()
{
	CriticalSectionScope lock(_requestsCriticalSection);
	TextureLoadingRequest* result = nullptr;

	if (_requests.size()) 
	{
		result = _requests.front();
		_requests.pop();
	}

	return result;
}

uint64_t TextureLoadingThread::main()
{
	while (running())
	{
		TextureLoadingRequest* req = dequeRequest();

		if (req)
		{
			req->textureDescription = loadTexture(req->fileName);

			Invocation1 invocation;
			invocation.setTarget(_delegate, &TextureLoadingThreadDelegate::textureLoadingThreadDidLoadTextureData, req);
			invocation.invokeInMainRunLoop();
		}
		else
		{
			suspend();
		}
	}

	return 0;
}

void TextureLoadingThread::addRequest(const std::string& fileName, Texture::Pointer texture,
	TextureLoaderDelegate* delegate)
{
	if (delegate)
	{
		Invocation1 i;
		i.setTarget(delegate, &TextureLoaderDelegate::textureDidStartLoading, texture);
		i.invokeInMainRunLoop();
	}
	
	CriticalSectionScope lock(_requestsCriticalSection);
	_requests.push(etCreateObject<TextureLoadingRequest>(fileName, texture, delegate));

	if (running())
		resume();
	else
		run();
}
