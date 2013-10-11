/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
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
		
		void start(const std::string& identifier, const std::string& signature);
		
		void show();
		void show(const std::string& tag);
		void showMoreGames();
		
	private:
		ChartBoostProxyPrivate* _private;
	};
}