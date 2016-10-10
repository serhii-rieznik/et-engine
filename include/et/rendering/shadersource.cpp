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

inline bool getIncludeFileName(const std::string& source, size_t& pos, std::string& fileName)
{
	pos += kInclude.length();

	fileName.clear();
	fileName.reserve(32);

	bool bracketOpen = false;
	while (pos < source.length())
	{
		char c = source[pos];
		if (((c == '\"') || (c == '<')) && !bracketOpen)
		{
			bracketOpen = true;
		}
		else if (bracketOpen && ((c == '\"') || (c == '<') || (c == '\n') || (c == '\r')))
		{
			++pos;
			break;
		}
		else if (bracketOpen)
		{
			fileName += c;
		}
		++pos;
	}

	fileName = application().resolveFileName(fileName);
	return fileExists(fileName);
}

void parseShaderSource(std::string& source, const std::string& baseFolder, const StringList& defines)
{
	if (source.empty())
		return;

	application().pushSearchPath(baseFolder);

	size_t includePos = std::string::npos;
	while ((includePos = source.find(kInclude)) != std::string::npos)
	{
		std::string includeName;
		size_t endPos = includePos;
		if (getIncludeFileName(source, endPos, includeName))
		{
			std::string include = loadTextFile(includeName);
			std::string after = source.substr(endPos);
			std::string before = source.substr(0, includePos);
			source = before +
				"\n// ------- begin auto included file --------\n" +
				include +
				"\n// -------- end auto included file ---------\n" +
				after;
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
