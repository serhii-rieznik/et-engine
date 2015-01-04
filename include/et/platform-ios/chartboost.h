/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/debug.h>
#include <et/core/singleton.h>

namespace et
{
	class ChartBoostProxyPrivate;
	class ChartBoostProxy : public et::Singleton<ChartBoostProxy>
	{
	public:
		ChartBoostProxy();
		~ChartBoostProxy()
		
		void start(const std::string& identifier, const std::string& signature);
		
		void show();
		void show(const std::string& tag);
		void showMoreGames();
		
	private:
		ET_DECLARE_PIMPL(ChartBoostProxy, 32)
	};
}