/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{

enum class ParseDirective : uint32_t
{
	None,
	Include,
	DefaultHeader,
	InputLayout,
	InputDefines,
	StageDefine,
	UserDefined,
	Options,
};

using ParseDirectiveCallback = std::function<void(ParseDirective, std::string&, uint32_t)>;

StringList parseShaderSource(std::string& source, const std::string& baseFolder,
	const StringList& defines, ParseDirectiveCallback cb, const Set<ParseDirective>& skipDirectives);

}
