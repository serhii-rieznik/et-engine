/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/apiobjects/textureloadingthread.h>
#include <et/resources/textureloader.h>
#include <et/app/invocation.h>

using namespace et;

TextureLoaderDelegate::~TextureLoaderDelegate()
{
	for (auto i : _requests)
		i->discardDelegate();
}

TextureLoadingRequest::TextureLoadingRequest(const std::string& name, const Texture& tex,
	TextureLoaderDelegate* d) : fileName(name), textureDescription(new TextureDescription),
	texture(tex), delegate(d)
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
	Thread(false), _delegate(delegate), _running(false)
{
}

TextureLoadingThread::~TextureLoadingThread()
{
	stop();

	CriticalSectionScope lock(_requestsCriticalSection);
	while (_requests.size())
	{
		delete _requests.front();
		_requests.pop();
	}
}

TextureLoadingRequest* TextureLoadingThread::dequeRequest()
{
	CriticalSectionScope lock(_requestsCriticalSection);
	TextureLoadingRequest* result = 0;

	if (_requests.size()) 
	{
		result = _requests.front();
		_requests.pop();
	}

	return result;
}

ThreadResult TextureLoadingThread::main()
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

void TextureLoadingThread::addRequest(const std::string& fileName, Texture texture,
	TextureLoaderDelegate* delegate)
{
	if (delegate)
	{
		Invocation1 i;
		i.setTarget(delegate, &TextureLoaderDelegate::textureDidStartLoading, texture);
		i.invokeInMainRunLoop();
	}
	
	CriticalSectionScope lock(_requestsCriticalSection);
	_requests.push(new TextureLoadingRequest(fileName, texture, delegate));

	if (running())
		resume();
	else
		run();
}
