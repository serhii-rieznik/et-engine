/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
inline bool platformHasHardwareKeyboard()
{
	return true;
}

enum class SelectFileMode : uint32_t
{
	Open,
	Save
};

enum class AlertType : uint32_t
{
	Information,
	Warning,
	Error
};

std::string selectFile(const StringList& types, SelectFileMode mode, const std::string& defaultName);
void alert(const std::string& title, const std::string& message, const std::string& button, AlertType type);
bool canOpenURL(const std::string&);
}
