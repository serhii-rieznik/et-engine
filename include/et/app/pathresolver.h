/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <deque>

namespace et
{
	class RenderContext;
	
	class PathResolver : public Shared
	{
	public:
		ET_DECLARE_POINTER(PathResolver);
		
	public:
		virtual std::string resolveFilePath(const std::string&) = 0;
		virtual std::string resolveFolderPath(const std::string&) = 0;
		virtual std::set<std::string> resolveFolderPaths(const std::string&) = 0;
		
		virtual ~PathResolver() { }
	};
	
	class StandardPathResolver : PathResolver
	{
	public:
		ET_DECLARE_POINTER(StandardPathResolver);
		
	public:
		void setRenderContext(RenderContext*);
		
		std::string resolveFilePath(const std::string&);
		std::string resolveFolderPath(const std::string&);
		std::set<std::string> resolveFolderPaths(const std::string&);
		
		void pushSearchPath(const std::string& path);
		void pushSearchPaths(const std::set<std::string>& path);
		void pushRelativeSearchPath(const std::string& path);
		
		void popSearchPaths(size_t = 1);
		
		void setSilentErrors(bool e)
			{ _silentErrors = e; }
		
	private:
		void validateCaches();
		
	private:
		RenderContext* _rc = nullptr;
		std::deque<std::string> _searchPath;
		
		std::string _cachedLang;
		std::string _cachedSubLang;
		std::string _cachedLanguage;
		std::string _cachedScreenScale;
		std::string _baseFolder;
		std::string _cachedLocale;
		
		float _cachedScreenScaleFactor = 0.0f;
		bool _silentErrors = false;
	};
}
