/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#define ET_INTERNAL_WRAP_LOCALIZEDPRINTF(expression)	\
	StringDataStorage buffer(1024 + key.size(), 0); \
	expression; return std::string(buffer.data());

namespace locale
{
#if (ET_SUPPORT_VARIADIC_TEMPLATES)
	
	template <typename ...args>
	std::string localizedPrintf(const std::string& key, args&&...a)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a...))
	}
	
#else
	
	template <typename A1>
	std::string localizedPrintf(const std::string& key, A1 a1)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1))
	}
	
	template <typename A1, typename A2>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2))
	}
	
	template <typename A1, typename A2, typename A3>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3))
	}
	
	template <typename A1, typename A2, typename A3, typename A4>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5, a6))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5, a6, a7))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5, a6, a7, a8))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5, a6, a7, a8, a9))
	}
	
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
	std::string localizedPrintf(const std::string& key, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10)
	{
		ET_INTERNAL_WRAP_LOCALIZEDPRINTF(sprintf(buffer.data(), localized(key).c_str(), a1, a2, a3, a4, a5, a6, a7, a8, a9, a10))
	}
	
#endif
}
