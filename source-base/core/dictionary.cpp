/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>
#include <et/core/dictionary.h>
#include <et/core/json.h>

using namespace et;

void printDictionary(const Dictionary& dict, const std::string& tabs);
void printArray(ArrayValue arr, const std::string& tabs);

VariantBase::Pointer duplicateValue(VariantBase::Pointer obj)
{
	if (obj->variantClass() == VariantClass::String)
		return StringValue(obj).duplicate();

	if (obj->variantClass() == VariantClass::Dictionary)
		return Dictionary(obj).duplicate();

	if (obj->variantClass() == VariantClass::Array)
		return ArrayValue(obj).duplicate();

	if (obj->variantClass() == VariantClass::Integer)
		return IntegerValue(obj).duplicate();

	if (obj->variantClass() == VariantClass::Boolean)
		return BooleanValue(obj).duplicate();

	if (obj->variantClass() == VariantClass::Float)
		return FloatValue(obj).duplicate();

	abort();
	return VariantBase::Pointer();
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

ArrayValue::VariantPointer ArrayValue::duplicate() const
{
	ArrayValue result;
	result->content.reserve(reference().content.size());
	for (const auto& val : reference().content)
	{
		result->content.push_back(duplicateValue(val));
	}
	return result;
}

VariantBase::Pointer Dictionary::baseValueForKeyPathInHolder(const StringList& path,
	VariantBase::Pointer holder) const
{
	if (holder->variantClass() == VariantClass::Dictionary)
	{
		Dictionary dictionary(holder);
		auto value = dictionary->content.find(path.front());
		
		if (value == dictionary->content.end())
			return Dictionary();
		
		return (path.size() == 1) ? value->second : baseValueForKeyPathInHolder(
			StringList(path.begin() + 1, path.end()), value->second);
	}
	else if (holder->variantClass() == VariantClass::Array)
	{
		size_t index = strToInt(path.front());
		ArrayValue array(holder);
		
		if (index >= array->content.size())
			return ArrayValue();
		
		VariantBase::Pointer value = array->content.at(index);
		return (path.size() == 1) ? value : baseValueForKeyPathInHolder(
			StringList(path.begin() + 1, path.end()), value);
	}
	else if (holder->variantClass() == VariantClass::String)
	{
		StringValue string(holder);
		log::warning("Trying to extract subvalue `%s` from string `%s`", path.front().c_str(),
			string->content.c_str());
	}
	else if (holder->variantClass() == VariantClass::Integer)
	{
		IntegerValue number(holder);
		log::warning("Trying to extract subvalue `%s` from number %lld.", path.front().c_str(), number->content);
	}
	else
	{
		ET_FAIL("Invalid value class.");
	}
	
	return VariantBase::Pointer();
}

VariantBase::Pointer Dictionary::objectForKeyPath(const StringList& path) const
{
	ET_ASSERT(path.size() > 0);
	return baseValueForKeyPathInHolder(path, *this);
}

VariantBase::Pointer Dictionary::objectForKey(const std::string& key) const
{
	if (hasKey(key))
		return reference().content.at(key);
	
	return Dictionary();
}

bool Dictionary::valueForKeyPathIsClassOf(const StringList& key, VariantClass c) const
{
	auto v = objectForKeyPath(key);
	return v.valid() && (v->variantClass() == c);
}

bool Dictionary::hasKey(const std::string& key) const
{
	return reference().content.count(key) > 0;
}

VariantClass Dictionary::VariantClassForKey(const std::string& key) const
{
	return hasKey(key) ? objectForKeyPath(StringList(1, key))->variantClass() : VariantClass::Invalid;
}

Dictionary::Dictionary(const std::string& jsonString)
{
	loadFromJson(jsonString);
}

bool Dictionary::loadFromJson(const std::string& jsonString)
{
	VariantClass vc = VariantClass::Invalid;
	Dictionary object = json::deserialize(jsonString, vc);
	if (vc == VariantClass::Dictionary)
    {
        reference().content = object->content;
        return true;
    }
    
    return false;
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

Dictionary::VariantPointer Dictionary::duplicate() const
{
	Dictionary result;
	for (const auto& kv : reference().content)
	{
		result.setValueForKey(kv.first, duplicateValue(kv.second));
	}
	return result;
}

void Dictionary::addKeyPathsFromHolder(VariantBase::Pointer holder, const std::string& baseKeyPath, StringList& keyPaths) const
{
	std::string nextKeyPath = baseKeyPath.empty() ? emptyString : (baseKeyPath + "/");
	if (holder->variantClass() == VariantClass::Dictionary)
	{
		Dictionary d(holder);
		for (auto& v : d->content)
		{
			std::string keyPath = nextKeyPath + v.first;
			keyPaths.push_back(keyPath);
			addKeyPathsFromHolder(v.second, keyPath, keyPaths);
		}
	}
	else if (holder->variantClass() == VariantClass::Array)
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
		if (i->variantClass() == VariantClass::Integer)
		{
			IntegerValue val = i;
			log::info("%s%lld", tabs.c_str(), val->content);
		}
		else if (i->variantClass() == VariantClass::Boolean)
		{
			BooleanValue val = i;
			log::info("%s%s", tabs.c_str(), val->content == 0 ? "false" : "true");
		}
		else if (i->variantClass() == VariantClass::Float)
		{
			FloatValue val = i;
			log::info("%s%f", tabs.c_str(), val->content);
		}
		else if (i->variantClass() == VariantClass::String)
		{
			StringValue val = i;
			log::info("%s\"%s\"", tabs.c_str(), val->content.c_str());
		}
		else if (i->variantClass() == VariantClass::Array)
		{
			log::info("%s{", tabs.c_str());
			printArray(i, tabs + "\t");
			log::info("%s}", tabs.c_str());
		}
		else if (i->variantClass() == VariantClass::Dictionary)
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
		if (i.second->variantClass() == VariantClass::Integer)
		{
			IntegerValue val = i.second;
			log::info("%s%s = %lld", tabs.c_str(), i.first.c_str(), val->content);
		}
		else if (i.second->variantClass() == VariantClass::Boolean)
		{
			BooleanValue val = i.second;
			log::info("%s%s = %s", tabs.c_str(), i.first.c_str(), val->content == 0 ? "false" : "true");
		}
		else if (i.second->variantClass() == VariantClass::Float)
		{
			FloatValue val = i.second;
			log::info("%s%s = %f", tabs.c_str(), i.first.c_str(), val->content);
		}
		else if (i.second->variantClass() == VariantClass::String)
		{
			StringValue val = i.second;
			log::info("%s%s = \"%s\"", tabs.c_str(), i.first.c_str(), val->content.c_str());
		}
		else if (i.second->variantClass() == VariantClass::Array)
		{
			ArrayValue val = i.second;
			log::info("%s%s =", tabs.c_str(), i.first.c_str());
			log::info("%s{", tabs.c_str());
			printArray(val, tabs + "\t");
			log::info("%s}", tabs.c_str());
		}
		else if (i.second->variantClass() == VariantClass::Dictionary)
		{
			Dictionary val = i.second;
			log::info("%s%s =", tabs.c_str(), i.first.c_str());
			log::info("%s<", tabs.c_str());
			printDictionary(val, tabs + "\t");
			log::info("%s>", tabs.c_str());
		}
	}
}
