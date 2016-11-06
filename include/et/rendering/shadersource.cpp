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
const std::string kDirectiveInputDefines = "inputdefines";
const std::string kDirectiveStageDefine = "stagedefine";

inline bool getDirective(const std::string& source, size_t& pos, std::string& fileName,
	ParseDirective& directive, const Set<ParseDirective>& skipDirectives)
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
		static const std::unordered_map<std::string, ParseDirective> validDirectives =
		{
			{ kDirectiveInputLayout, ParseDirective::InputLayout },
			{ kDirectiveDefaultHeader, ParseDirective::DefaultHeader },
			{ kDirectiveStageDefine, ParseDirective::StageDefine },
			{ kDirectiveInputDefines, ParseDirective::InputDefines },
		};

		auto i = validDirectives.find(fileName);
		directive = (i == validDirectives.end()) ? ParseDirective::UserDefined : i->second;
		return skipDirectives.count(directive) == 0;
	}

	directive = ParseDirective::Include;
	fileName = application().resolveFileName(fileName);
	return fileExists(fileName);
}

void parseShaderSource(std::string& source, const std::string& baseFolder, const StringList& defines,
	ParseDirectiveCallback cb, const Set<ParseDirective>& skipDirectives)
{
	if (source.empty())
		return;

	application().pushSearchPath(baseFolder);

	bool sourceModified = false;
	Set<size_t> skippedLocations = { std::string::npos };
	do
	{
		sourceModified = false;
		for (size_t offset = 0; offset < source.size(); )
		{
			size_t includePos = source.find(kInclude, offset);
			if (skippedLocations.count(includePos) == 0)
			{
				sourceModified = true;
				std::string includeName;
				size_t endPos = includePos;
				ParseDirective directive = ParseDirective::None;
				if (getDirective(source, endPos, includeName, directive, skipDirectives))
				{
					sourceModified = true;
					source.erase(includePos, endPos - includePos);
					if (directive == ParseDirective::Include)
					{
						source.insert(includePos, loadTextFile(includeName));
					}
					else if (directive == ParseDirective::InputDefines)
					{
						if (!defines.empty())
						{
							std::string definesString;
							definesString.reserve(64 * defines.size());
							for (const std::string& def : defines)
								definesString += def;
							source.insert(includePos, definesString);
						}
					}
					else
					{
						cb(directive, source, static_cast<uint32_t>(includePos));
					}
				}
				else
				{
					skippedLocations.insert(includePos);
					offset = includePos + 1;
				}
			}
			else
			{
				if (includePos == std::string::npos)
					break;

				offset = includePos + 1;
			}
		}
	}
	while (sourceModified);

	application().popSearchPaths();
}

}
