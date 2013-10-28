/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	namespace social
	{
		bool canTweet();
		bool canPostToFacebook();
		
		void tweet(const std::string& text, const std::string& pathToImage, const std::string& url);
		void postToFacebook(const std::string& text, const std::string& pathToImage, const std::string& url);
	}
}
