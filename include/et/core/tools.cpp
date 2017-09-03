/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/datastorage.h>
#include <et/core/tools.h>
#include <et/core/cout.h>

namespace et
{

intptr_t streamSize(std::istream& s)
{
	std::streamoff currentPos = s.tellg();
	
	s.seekg(0, std::ios::end);
	std::streamoff endPos = s.tellg();
	s.seekg(currentPos, std::ios::beg);
	
	return static_cast<intptr_t>(endPos);
}

std::string loadTextFile(const std::string& fileName)
{
	InputStream file(fileName, StreamMode_Binary);
	if (file.invalid()) return emptyString;

	StringDataStorage data(streamSize(file.stream()) + 1, 0);
	file.stream().read(data.data(), data.size());
	
	return std::string(data.data());
}

std::string addTrailingSlash(const std::string& path)
{
	if (path.empty() || (path.back() == pathDelimiter))
	{
		return path;
	}
	else if (path.back() == invalidPathDelimiter)
	{
		auto result = path;
		*result.rbegin() = pathDelimiter;
		return result;
	}
	else
	{
		return path + pathDelimiter;
	}
}

std::string replaceFileExt(const std::string& fileName, const std::string& newExt)
{
	std::string name = getFileName(fileName);
	std::string path = getFilePath(fileName);

	size_t dotPos = name.find_last_of(".");
	if (dotPos == std::string::npos)
		return fileName + newExt;

	name.erase(dotPos);
	return path + name + newExt;
}

std::string removeFileExt(const std::string& fileName)
{
	std::string name = getFileName(fileName);
	std::string path = getFilePath(fileName);

	size_t dotPos = name.find_last_of(".");
	if (dotPos == std::string::npos)
		return fileName;

	name.erase(dotPos, name.length() - dotPos);
	return path + name;
}

std::string& trim(std::string& str)
{
	size_t strSize = str.length();
	if (strSize == 0)
		return str;

	size_t leadingWhitespace = 0;
	size_t trailingWhitespace = 0;
	size_t pos = 0;
	while ((pos < strSize) && isWhitespaceChar(str[pos++])) 
		++leadingWhitespace;

	pos = strSize - 1;
	while ((pos > 0) && isWhitespaceChar(str[pos--])) 
		++trailingWhitespace;

	if (trailingWhitespace)
		str.erase(str.length() - trailingWhitespace);
	
	if (leadingWhitespace)
		str.erase(0, leadingWhitespace);

	return str;
}

std::string capitalize(std::string v)
{
	v[0] = ::toupper(v[0]) & 0xff;
	for (size_t i = 1; i < v.length(); ++i)
	{
		if (v[i-1] == ET_SPACE)
			v[i] = ::toupper(v[i]) & 0xff;
	}
	return v;
}

bool isUtf8String(const std::string& s)
{
	for (auto c : s)
	{
		if ((c & 0x80) != 0)
			return true;
	}
	
	return false;
}

std::string getFilePath(const std::string& name)
{
	std::string::size_type p = normalizeFilePath(name).find_last_of(pathDelimiter);
	return (p == std::string::npos) ? emptyString : name.substr(0, ++p);
}

std::string getFileFolder(const std::string& name)
{
	std::string::size_type p = normalizeFilePath(name).find_last_of(pathDelimiter);
	return (p == std::string::npos) ? emptyString : name.substr(0, p);
}

std::string getFileName(const std::string& fullPath)
{
	std::string::size_type p = normalizeFilePath(fullPath).find_last_of(pathDelimiter);
	return (p  == std::string::npos) ? fullPath : fullPath.substr(p + 1);
}

std::string removeUpDir(std::string name)
{
	std::string::size_type dotsPos = name.find("..");
	
	if (dotsPos != std::string::npos)
		name = getFilePath(name.substr(0, dotsPos - 1)) + name.substr(dotsPos + 3);
	
	return name;
}

std::string normalizeFilePath(const std::string& s)
{
	auto result = s;
	normalizeFilePath(result);
	return result;
}

std::string& normalizeFilePath(std::string& s)
{
	for (char& i : s)
	{
		if (i == invalidPathDelimiter)
			i = pathDelimiter;
	}
	return s;
}

std::string getFileExt(std::string name)
{
	name = normalizeFilePath(name);

	size_t dotPos = name.find_last_of('.');
	if (dotPos == std::string::npos)
		return emptyString;
	
	size_t slashPos = name.find_last_of(pathDelimiter);
	if (slashPos == std::string::npos)
		return name.substr(dotPos + 1);
	
	if (dotPos > slashPos)
		return name.substr(dotPos + 1);
	
	return emptyString;
}

float extractFloat(std::string& s)
{
	size_t len = s.length();
	const char* data = s.c_str();
	bool hasMinus = data[0] == '-';
	bool dotFound = false;
	size_t offset = static_cast<size_t>(hasMinus);
	float value = 0.0f;
	float scale = 1.0f;
	for (; offset < len; ++offset)
	{
		char c = data[offset];
		if (((c >= '0') && (c <= '9')) || (c == '.'))
		{
			if (c == '.')
			{
				if (dotFound) break;
				dotFound = true;
				scale = 0.1f;
			}
			else 
			{
				float cValue = static_cast<float>(c - '0');
				if (dotFound)
				{
					value += cValue * scale;
					scale /= 10.0f;
				}
				else
				{
					value = value * 10.0f + cValue;
					scale *= 10.0f;
				}
			}
		}
		else 
		{
			break;
		}
	}

	s.erase(0, offset);
	return hasMinus ? -value : value;
}

StringList split(const std::string& s, const std::string& delim)
{
	StringList result;
	result.reserve(1 + s.size() / 2);
	
	size_t startIndex = 0;
	size_t separatorIndex = s.find_first_of(delim, startIndex);

	while (separatorIndex != std::string::npos)
	{
		if (separatorIndex >= startIndex)
			result.push_back(s.substr(startIndex, separatorIndex - startIndex));

		startIndex = separatorIndex + 1;
		separatorIndex = s.find_first_of(delim, startIndex);
	}

	if (startIndex < s.size())
		result.push_back(s.substr(startIndex));

	return result;
}

std::ostream& operator << (std::ostream& stream, const StringList& list)
{
	stream << "{" << std::endl;
	for (const auto& i : list)
		stream << "\t" << i << std::endl;
	stream << "}" << std::endl;
	
	return stream;
}

std::string removeWhitespace(const std::string& s)
{
    std::string result(s.size(), 0);
    size_t i = 0;
    for (char c : s)
    {
        if (!isWhitespaceChar(c))
            result[i++] = c;
    }
    return result;
}

StringList parseDefines(std::string defines, std::string separators)
{
    StringList result;
    
    auto addDefine = [&result](std::string& define)
    {
        if (define.empty()) return;
        
        std::transform(define.begin(), define.end(), define.begin(), [](char c)
                       { return (c == '=') ? ' ' : c; });
        result.push_back("#define " + define + "\n");
    };
    
    if (separators.length() == 0)
        separators = ",; \t";
    
    while (defines.length() > 0)
    {
        std::string::size_type separator_pos = std::string::npos;
        
        for (auto& s_i : separators)
        {
            std::string::size_type s = defines.find_first_of(s_i);
            if (s != std::string::npos)
            {
                separator_pos = s;
                break;
            }
        }
        
        if (separator_pos == std::string::npos)
        {
            addDefine(defines);
            break;
        }
        else
        {
            std::string define = defines.substr(0, separator_pos);
            defines.erase(0, separator_pos + 1);
            addDefine(define);
        }
    }
    
    return result;
}
    
void parseIncludes(std::string& source, const std::string& workFolder)
{
    std::string::size_type ip = source.find("#include");
    
    while (ip != std::string::npos)
    {
        std::string before = source.substr(0, ip);
        
        source.erase(0, before.size());
        std::string ifname = source.substr(0, source.find_first_of(char(10)));
        source.erase(0, ifname.size());
        std::string after = source.substr();
        
        if (ifname.find_first_of('"') != std::string::npos)
        {
            ifname.erase(0, ifname.find_first_of('"') + 1);
            ifname.erase(ifname.find_last_of('"'));
        }
        else
        {
            ifname.erase(0, ifname.find_first_of('<') + 1);
            ifname.erase(ifname.find_last_of('>'));
        }
        
        std::string baseName = removeUpDir(workFolder + ifname);
        while (baseName.find("..") != std::string::npos)
		{
            baseName = removeUpDir(baseName);
		}
		
        if (fileExists(baseName))
        {
			std::string include = loadTextFile(baseName);
			source = before + include + after;
		}
        else
        {
            log::error("failed to include %s, starting from folder %s", ifname.c_str(), workFolder.c_str());
        }
        
        ip = source.find("#include");
    }
}

}
