/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/properties.h>
#include <et/core/singleton.h>
#include <et/core/containers.h>
#include <et/app/events.h>

namespace et
{
	class DownloadManager;
	class DownloadManagerPrivate;
	
	class DownloadRequest : public Shared
	{
	public:
		typedef IntrusivePtr<DownloadRequest> Pointer;

		ET_DECLARE_EVENT1(progress, DownloadRequest::Pointer)
		ET_DECLARE_EVENT1(completed, DownloadRequest::Pointer);
		ET_DECLARE_EVENT1(failed, DownloadRequest::Pointer);

	public:
		DownloadRequest(const std::string&, const std::string&);
		DownloadRequest(const std::string&);
		~DownloadRequest();

		uint64_t totalSize() const
			{ return _totalSize; }

		uint64_t downloaded() const
			{ return _downloaded; }

		ET_DECLARE_PROPERTY_GET_REF(std::string, url)
		ET_DECLARE_PROPERTY_GET_REF(std::string, destination)
		ET_DECLARE_PROPERTY_GET_REF(BinaryDataStorage, data)

	private:
		friend class DownloadManager;
		friend class DownloadThread;
		
		friend size_t writeCallback(void*, size_t, size_t, DownloadRequest*);
		friend int progessCallback(void*, double, double, double, double);
		
		size_t appendData(void* ptr, size_t, size_t);
		void cleanup();

	private:
		FILE* _destFile = nullptr;
		uint64_t _totalSize = 0;
		uint64_t _downloaded = 0;
	};
	
	class DownloadManager : public et::Singleton<DownloadManager>
	{
	public:
		DownloadRequest::Pointer downloadFile(const std::string& url, const std::string& destination);
		DownloadRequest::Pointer downloadFile(const std::string& url);
		
	public:
		ET_DECLARE_EVENT1(downloadCompleted, DownloadRequest::Pointer)
		ET_DECLARE_EVENT1(downloadFailed, DownloadRequest::Pointer)
		
	private:
		DownloadManager();
		~DownloadManager();
		
		ET_SINGLETON_COPY_DENY(DownloadManager)
		
	private:
		ET_DECLARE_PIMPL(DownloadManager, 512)
	};
}
