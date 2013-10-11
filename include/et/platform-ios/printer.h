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
	class PrinterPrivate;
	class Printer
	{
	public:
		Printer();
		~Printer();

		void printImageFromFile(const std::string&);
		
	private:
		PrinterPrivate* _private;
	};
}
