/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>
#include <et/core/dictionary.h>
#include <et/json/json.h>

using namespace et;

void printDictionary(const Dictionary& dict, const std::string& tabs);
void printArray(ArrayValue arr, const std::string& tabs);

ValueBase::Pointer duplicateValue(ValueBase::Pointer obj)
{
	if (obj->valueClass() == ValueClass_String)
		return StringValue(obj).duplicate();

	if (obj->valueClass() == ValueClass_Dictionary)
		return Dictionary(obj).duplicate();

	if (obj->valueClass() == ValueClass_Array)
		return ArrayValue(obj).duplicate();

	if (obj->valueClass() == ValueClass_Integer)
		return IntegerValue(obj).duplicate();

	if (obj->valueClass() == ValueClass_Boolean)
		return BooleanValue(obj).duplicate();

	if (obj->valueClass() == ValueClass_Float)
		return FloatValue(obj).duplicate();

	abort();
	return ValueBase::Pointer();
}

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

ArrayValue::ValuePointer ArrayValue::duplicate() const
{
	ArrayValue result;
	result->content.reserve(reference().content.size());
	for (const auto& val : reference().content)
	{
		result->content.push_back(duplicateValue(val));
	}
	return result;
}

ValueBase::Pointer Dictionary::baseValueForKeyPathInHolder(const std::vector<std::string>& path,
	ValueBase::Pointer holder) const
{
	if (holder->valueClass() == ValueClass_Dictionary)
	{
		Dictionary dictionary(holder);
		auto value = dictionary->content.find(path.front());
		
		if (value == dictionary->content.end())
			return Dictionary();
		
		return (path.size() == 1) ? value->second : baseValueForKeyPathInHolder(
			std::vector<std::string>(path.begin() + 1, path.end()), value->second);
	}
	else if (holder->valueClass() == ValueClass_Array)
	{
		size_t index = strToInt(path.front());
		ArrayValue array(holder);
		
		if (index >= array->content.size())
			return ArrayValue();
		
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
	if (hasKey(key))
		return reference().content.at(key);
	
	return Dictionary();
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

Dictionary::Dictionary(const std::string& jsonString)
{
	loadFromJson(jsonString);
}

bool Dictionary::loadFromJson(const std::string& jsonString)
{
	ValueClass vc = ValueClass_Invalid;
	Dictionary object = json::deserialize(jsonString, vc);
	if (vc != ValueClass_Dictionary)
		return false;
	
	reference().content = object->content;
	return true;
}

std::string Dictionary::storeToJson() const
{
	return json::serialize(*this);
}

StringList Dictionary::allKeyPaths()
{
	StringList result;
	addKeyPathsFromHolder(*this, emptyString, result);
	return result;
}

Dictionary::ValuePointer Dictionary::duplicate() const
{
	Dictionary result;
	for (const auto& kv : reference().content)
	{
		result.setValueForKey(kv.first, duplicateValue(kv.second));
	}
	return result;
}

void Dictionary::addKeyPathsFromHolder(ValueBase::Pointer holder, const std::string& baseKeyPath, StringList& keyPaths) const
{
	std::string nextKeyPath = baseKeyPath.empty() ? emptyString : (baseKeyPath + "/");
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
		else if (i->valueClass() == ValueClass_Boolean)
		{
			BooleanValue val = i;
			log::info("%s%s", tabs.c_str(), val->content == 0 ? "false" : "true");
		}
		else if (i->valueClass() == ValueClass_Float)
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

void printDictionary(const Dictionary& dict, const std::string& tabs)
{
	for (auto i : dict->content)
	{
		if (i.second->valueClass() == ValueClass_Integer)
		{
			IntegerValue val = i.second;
			log::info("%s%s = %lld", tabs.c_str(), i.first.c_str(), val->content);
		}
		else if (i.second->valueClass() == ValueClass_Boolean)
		{
			BooleanValue val = i.second;
			log::info("%s%s = %s", tabs.c_str(), i.first.c_str(), val->content == 0 ? "false" : "true");
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
			log::info("%s%s =", tabs.c_str(), i.first.c_str());
			log::info("%s{", tabs.c_str());
			printArray(val, tabs + "\t");
			log::info("%s}", tabs.c_str());
		}
		else if (i.second->valueClass() == ValueClass_Dictionary)
		{
			Dictionary val = i.second;
			log::info("%s%s =", tabs.c_str(), i.first.c_str());
			log::info("%s<", tabs.c_str());
			printDictionary(val, tabs + "\t");
			log::info("%s>", tabs.c_str());
		}
	}
}
