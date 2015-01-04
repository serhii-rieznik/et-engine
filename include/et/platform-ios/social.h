/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/app/events.h>

namespace et
{
	namespace social
	{
		bool canTweet();
		bool canPostToFacebook();
		
		void tweet(const std::string& text, const std::string& pathToImage, const std::string& url);
		void postToFacebook(const std::string& text, const std::string& pathToImage, const std::string& url);
		
		struct notifications
		{
			static Event1<bool> sharingFinished;
		};
	}
}
