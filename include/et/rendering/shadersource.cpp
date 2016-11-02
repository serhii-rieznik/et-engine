/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/shadersource.h>

namespace et
{

const std::string kInclude = "#include";
const std::string kDirectiveDefaultHeader = "et";
const std::string kDirectiveInputLayout = "inputlayout";

inline bool getIncludeFileName(const std::string& source, size_t& pos, std::string& fileName,
	ParseDirective& directive)
{
	pos += kInclude.length();

	fileName.clear();
	fileName.reserve(32);

	bool bracketOpen = false;
	bool isDirective = false;
	while (pos < source.length())
	{
		char c = source[pos];
		if (((c == '\"') || (c == '<')) && !bracketOpen)
		{
			bracketOpen = true;
			isDirective = (c == '<');
		}
		else if (bracketOpen && ((c == '\"') || (c == '>') || (c == '\n') || (c == '\r')))
		{
			++pos;
			if (isDirective && (c != '>'))
			{
				ET_FAIL("Unmatched '<' symbol in source code");
			}
			break;
		}
		else if (bracketOpen)
		{
			fileName += c;
		}
		++pos;
	}

	if (isDirective)
	{
		directive = ParseDirective::UserDefined;

		if (fileName == kDirectiveInputLayout)
			directive = ParseDirective::InputLayout;

		if (fileName == kDirectiveDefaultHeader)
			directive = ParseDirective::DefaultHeader;

		return true;
	}

	directive = ParseDirective::Include;
	fileName = application().resolveFileName(fileName);
	return fileExists(fileName);
}

void parseShaderSource(std::string& source, const std::string& baseFolder, const StringList& defines,
	ParseDirectiveCallback cb)
{
	if (source.empty())
		return;

	application().pushSearchPath(baseFolder);

	size_t includePos = std::string::npos;
	while ((includePos = source.find(kInclude)) != std::string::npos)
	{
		std::string includeName;
		size_t endPos = includePos;
		ParseDirective directive = ParseDirective::None;
		if (getIncludeFileName(source, endPos, includeName, directive))
		{
			if (directive == ParseDirective::Include)
			{
				std::string before = source.substr(0, includePos);
				std::string include = loadTextFile(includeName);
				std::string after = source.substr(endPos);

				source = before +
					"\n// ------- begin auto included file --------\n" +
					include +
					"\n// -------- end auto included file ---------\n" +
					after;
			}
			else
			{
				source.erase(includePos, endPos - includePos);
				cb(directive, source, static_cast<uint32_t>(includePos));
			}
		}
		else
		{
			ET_FAIL("Failed to preprocess shader source");
		}
	}

	application().popSearchPaths();

	std::string definesString;
	definesString.reserve(64 * defines.size());
	for (const std::string& def : defines)
		definesString += def;

	source.insert(0, definesString);
}

}
