/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <queue>
#include <libcurl/curl.h>
#include <et/threading/thread.h>
#include <et/networking/downloadmanager.h>

using namespace et;

class et::DownloadThread : public Thread
{
public:
	DownloadThread(DownloadManager* owner) :
		Thread(false), _owner(owner) { }
	~DownloadThread();
	
	ThreadResult main();
	
	void pushRequest(DownloadRequest::Pointer);
	
private:
	DownloadRequest::Pointer dequeRequest();
	void processRequest(DownloadRequest::Pointer);
	
private:
	DownloadManager* _owner;
	
	CriticalSection _csModifying;
	std::queue<DownloadRequest::Pointer> _queue;
};

class et::DownloadManagerPrivate
{
public:
	DownloadManager* owner;
	DownloadThread* thread;
	
public:
	DownloadManagerPrivate(DownloadManager* o) :
		owner(o), thread(new DownloadThread(o)) { }
	
	~DownloadManagerPrivate()
	{
		thread->stop();
		thread->waitForTermination();
		delete thread;
	}
};

/**
 *
 * DownloadManager
 *
 */

DownloadManager::DownloadManager()
{
	_private = new DownloadManagerPrivate(this);
}

DownloadManager::~DownloadManager()
{
	delete _private;
}

DownloadRequest::Pointer DownloadManager::downloadFile(const std::string& url,
	const std::string& destination)
{
	DownloadRequest::Pointer request(new DownloadRequest(url, destination));
	_private->thread->pushRequest(request);
	return request;
}

DownloadRequest::Pointer DownloadManager::downloadFile(const std::string& url)
{
	DownloadRequest::Pointer request(new DownloadRequest(url));
	_private->thread->pushRequest(request);
	return request;
}

/**
 *
 * DownloadThread
 *
 */
ThreadResult DownloadThread::main()
{
	while (running())
	{
		DownloadRequest::Pointer req = dequeRequest();
		
		if (req.valid())
		{
			processRequest(req);
		}
		else
		{
			suspend();
		}
	}
	
	return 0;
}

DownloadThread::~DownloadThread()
{
	CriticalSectionScope lock(_csModifying);
	
	while (_queue.size())
		_queue.pop();
}

void DownloadThread::pushRequest(DownloadRequest::Pointer req)
{
	CriticalSectionScope lock(_csModifying);
	_queue.push(req);
	
	if (running())
		resume();
	else
		run();
}

DownloadRequest::Pointer DownloadThread::dequeRequest()
{
	CriticalSectionScope lock(_csModifying);
	DownloadRequest::Pointer result;
	if (_queue.size())
	{
		result = _queue.front();
		_queue.pop();
	}
	return result;
}

size_t et::writeCallback(void* ptr, size_t size, size_t nmemb, DownloadRequest* request)
{
	return request->appendData(ptr, size, nmemb);
}

int et::progessCallback(void* p, double dltotal, double dlnow, double, double)
{
	if (dltotal > 0.0)
	{
		DownloadRequest* req = reinterpret_cast<DownloadRequest*>(p);
		req->_totalSize = static_cast<uint64_t>(dltotal);
		req->_downloaded = static_cast<uint64_t>(dlnow);
		req->progress.invokeInMainRunLoop(DownloadRequest::Pointer(req));
	}
	return 0;
}

void DownloadThread::processRequest(DownloadRequest::Pointer request)
{
    CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, request->url().c_str());
	curl_easy_setopt(curl, CURLOPT_FILE, request.ptr());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progessCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, request.ptr());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

	CURLcode result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	request->cleanup();
	
	if (result == CURLE_OK)
	{
		request->completed.invokeInMainRunLoop(request);
		_owner->downloadCompleted.invokeInMainRunLoop(request);
	}
	else
	{
		request->failed.invokeInMainRunLoop(request);
		_owner->downloadFailed.invokeInMainRunLoop(request);
	}
}

/**
 *
 * DownloadRequest
 *
 */
DownloadRequest::DownloadRequest(const std::string& u, const std::string& d) :
	_url(u), _destination(d), _destFile(0)
{
	std::string folder = getFilePath(_destination);
	
	if (!folderExists(folder))
		createDirectory(folder, true);
	
	_destFile = fopen(_destination.c_str(), "wb");
	ET_ASSERT(_destFile != nullptr);
}

DownloadRequest::DownloadRequest(const std::string& u) :
	_url(u), _destFile(0), _totalSize(0), _downloaded(0) { }

DownloadRequest::~DownloadRequest()
	{ cleanup(); }

void DownloadRequest::cleanup()
{
	if (_destFile)
	{
		fclose(_destFile);
		_destFile = nullptr;
	}
}

size_t DownloadRequest::appendData(void* ptr, size_t n, size_t size)
{
	ET_ASSERT(ptr != nullptr);
	size_t actualDataSize = n * size;

	if (_destFile == nullptr)
	{
		_data.appendData(ptr, actualDataSize);
		return actualDataSize;
	}
	else
	{
		return fwrite(ptr, n, size, _destFile);
	}
}
