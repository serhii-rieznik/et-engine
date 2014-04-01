/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/et.h>
#include <et/core/dictionary.h>

using namespace et;

void printDictionary(Dictionary dict, const std::string& tabs);
void printArray(ArrayValue arr, const std::string& tabs);

void Dictionary::printContent() const
{
	log::info("<");
	printDictionary(*this, "\t");
	log::info(">");
}

void ArrayValue::printContent() const
{
	log::info("{");
	printArray(*this, "\t");
	log::info("}");
}

ValueBase::Pointer Dictionary::baseValueForKeyPathInHolder(const std::vector<std::string>& path,
	ValueBase::Pointer holder) const
{
	if (holder->valueClass() == ValueClass_Dictionary)
	{
		Dictionary dictionary(holder);
		auto value = dictionary->content.find(path.front());
		
		if (value == dictionary->content.end())
			return ValueBase::Pointer();
		
		return (path.size() == 1) ? value->second : baseValueForKeyPathInHolder(
			std::vector<std::string>(path.begin() + 1, path.end()), value->second);
	}
	else if (holder->valueClass() == ValueClass_Array)
	{
		size_t index = strToInt(path.front());
		ArrayValue array(holder);
		
		if (index >= array->content.size())
			return ValueBase::Pointer();
		
		ValueBase::Pointer value = array->content.at(index);
		
		return (path.size() == 1) ? value : baseValueForKeyPathInHolder(
			std::vector<std::string>(path.begin() + 1, path.end()), value);
	}
	else if (holder->valueClass() == ValueClass_String)
	{
		StringValue string(holder);
		log::warning("Trying to extract subvalue `%s` from string `%s`", path.front().c_str(),
			string->content.c_str());
	}
	else if (holder->valueClass() == ValueClass_Integer)
	{
		IntegerValue number(holder);
		log::warning("Trying to extract subvalue `%s` from number %lld.", path.front().c_str(), number->content);
	}
	else
	{
		ET_FAIL("Invalid value class.");
	}
	
	return ValueBase::Pointer();
}

ValueBase::Pointer Dictionary::objectForKeyPath(const std::vector<std::string>& path) const
{
	ET_ASSERT(path.size() > 0);
	return baseValueForKeyPathInHolder(path, *this);
}

ValueBase::Pointer Dictionary::objectForKey(const std::string& key) const
{
	return hasKey(key) ? reference().content.at(key) : ValueBase::Pointer();
}

bool Dictionary::valueForKeyPathIsClassOf(const std::vector<std::string>& key, ValueClass c) const
{
	auto v = objectForKeyPath(key);
	return v.valid() && (v->valueClass() == c);
}

bool Dictionary::hasKey(const std::string& key) const
{
	return reference().content.count(key) > 0;
}

ValueClass Dictionary::valueClassForKey(const std::string& key) const
{
	return hasKey(key) ? objectForKeyPath(StringList(1, key))->valueClass() : ValueClass_Invalid;
}

StringList Dictionary::allKeyPaths()
{
	StringList result;
	addKeyPathsFromHolder(*this, std::string(), result);
	return result;
}

void Dictionary::addKeyPathsFromHolder(ValueBase::Pointer holder, const std::string& baseKeyPath, StringList& keyPaths) const
{
	std::string nextKeyPath = baseKeyPath.empty() ? std::string() : (baseKeyPath + "/");
	if (holder->valueClass() == ValueClass_Dictionary)
	{
		Dictionary d(holder);
		for (auto& v : d->content)
		{
			std::string keyPath = nextKeyPath + v.first;
			keyPaths.push_back(keyPath);
			addKeyPathsFromHolder(v.second, keyPath, keyPaths);
		}
	}
	else if (holder->valueClass() == ValueClass_Array)
	{
		size_t index = 0;
		ArrayValue a(holder);
		for (auto& v : a->content)
		{
			std::string keyPath = nextKeyPath + intToStr(index++);
			keyPaths.push_back(keyPath);
			addKeyPathsFromHolder(v, keyPath, keyPaths);
		}
	}
}

/*
 * Service functions
 */
void printArray(ArrayValue arr, const std::string& tabs)
{
	for (auto i : arr->content)
	{
		if (i->valueClass() == ValueClass_Integer)
		{
			IntegerValue val = i;
			log::info("%s%lld", tabs.c_str(), val->content);
		}
		if (i->valueClass() == ValueClass_Float)
		{
			FloatValue val = i;
			log::info("%s%f", tabs.c_str(), val->content);
		}
		else if (i->valueClass() == ValueClass_String)
		{
			StringValue val = i;
			log::info("%s\"%s\"", tabs.c_str(), val->content.c_str());
		}
		else if (i->valueClass() == ValueClass_Array)
		{
			log::info("%s{", tabs.c_str());
			printArray(i, tabs + "\t");
			log::info("%s}", tabs.c_str());
		}
		else if (i->valueClass() == ValueClass_Dictionary)
		{
			Dictionary val = i;
			log::info("%s<", tabs.c_str());
			printDictionary(val, tabs + "\t");
			log::info("%s>", tabs.c_str());
		}
	}
}

void printDictionary(Dictionary dict, const std::string& tabs)
{
	for (auto i : dict->content)
	{
		if (i.second->valueClass() == ValueClass_Integer)
		{
			IntegerValue val = i.second;
			log::info("%s%s = %lld", tabs.c_str(), i.first.c_str(), val->content);
		}
		else if (i.second->valueClass() == ValueClass_Float)
		{
			FloatValue val = i.second;
			log::info("%s%s = %f", tabs.c_str(), i.first.c_str(), val->content);
		}
		else if (i.second->valueClass() == ValueClass_String)
		{
			StringValue val = i.second;
			log::info("%s%s = \"%s\"", tabs.c_str(), i.first.c_str(), val->content.c_str());
		}
		else if (i.second->valueClass() == ValueClass_Array)
		{
			ArrayValue val = i.second;
			log::info("%s%s =\n%s{", tabs.c_str(), i.first.c_str(), tabs.c_str());
			printArray(val, tabs + "\t");
			log::info("%s}", tabs.c_str());
		}
		else if (i.second->valueClass() == ValueClass_Dictionary)
		{
			Dictionary val = i.second;
			log::info("%s%s =\n%s<", tabs.c_str(), i.first.c_str(), tabs.c_str());
			printDictionary(val, tabs + "\t");
			log::info("%s>", tabs.c_str());
		}
	}
}
