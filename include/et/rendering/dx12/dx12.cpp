/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12.h>
#include <comdef.h>

namespace et
{
namespace dx12
{

std::string resultToString(HRESULT hr)
{
	char buffer[1024] = {};
	uint32_t offset = sprintf(buffer, "HRESULT 0x%08X: ", hr);
	try
	{
		DWORD ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			buffer + offset, sizeof(buffer) - 1 - offset, nullptr);
	}
	catch (...)
	{
	}
	return buffer;
}

}
}
