/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <external/jansson/jansson.h>
#include <et/core/json.h>

using namespace et;
using namespace et::json;

et::VariantBase::Pointer deserializeJson(const char*, size_t, VariantClass&, bool);

ArrayValue deserializeArray(json_t* json);
Dictionary deserializeDictionary(json_t* json);
StringValue deserializeString(json_t* json);
IntegerValue deserializeInteger(json_t* json);
FloatValue deserializeFloat(json_t* json);

json_t* serializeFloat(const FloatValue&);
json_t* serializeArray(const ArrayValue&);
json_t* serializeString(const StringValue&);
json_t* serializeInteger(const IntegerValue&);
json_t* serializeBoolean(const BooleanValue&);
json_t* serializeDictionary(const Dictionary&);

json_t* serializeValue(const VariantBase::Pointer&);

std::string et::json::serialize(const Dictionary& msg, size_t inFlags)
{
	json_t* dictionary = serializeDictionary(msg);
	
	size_t flags = JSON_PRESERVE_ORDER;
	flags |= (inFlags & SerializationFlag_ReadableFormat) ? JSON_INDENT(2) : JSON_COMPACT;
	flags |= (inFlags & SerializationFlag_ConvertUnicode) ? JSON_ENSURE_ASCII : 0;

	char* dump = json_dumps(dictionary, flags);
	
	std::string serialized(dump);
	
	free(dump);
	json_decref(dictionary);

	return serialized;
}

std::string et::json::serialize(const et::ArrayValue& arr, size_t inFlags)
{
	json_t* dictionary = serializeArray(arr);
	
	size_t flags = (inFlags & SerializationFlag_ReadableFormat) ? JSON_INDENT(2) : JSON_COMPACT;
	flags |= (inFlags & SerializationFlag_ConvertUnicode) ? JSON_ENSURE_ASCII : 0;
	
	char* dump = json_dumps(dictionary, flags);
	
	std::string serialized(dump);
	
	free(dump);
	json_decref(dictionary);
	
	return serialized;
}

et::VariantBase::Pointer et::json::deserialize(const char* input, size_t len, VariantClass& c, bool printErrors)
	{ return deserializeJson(input, len, c, printErrors); }

et::VariantBase::Pointer et::json::deserialize(const char* input, VariantClass& c, bool printErrors)
	{ return deserializeJson(input, strlen(input), c, printErrors); }

et::VariantBase::Pointer  et::json::deserialize(const std::string& s, VariantClass& c, bool printErrors)
	{ return deserializeJson(s.c_str(), s.length(), c, printErrors); }

et::VariantBase::Pointer deserializeJson(const char* buffer, size_t len, VariantClass& c, bool printErrors)
{
	c = VariantClass::Invalid;
	
	if ((buffer == nullptr) || (len == 0))
		return Dictionary();
	
	json_error_t error = { };
	json_t* root = json_loadb(buffer, len, 0, &error);
	
	if (error.line != -1)
	{
		if (printErrors)
		{
			log::error("JSON parsing error (%d,%d): %s %s", error.line, error.column, error.source, error.text);
			log::error("%s", buffer);
		}
		json_decref(root);
		return Dictionary();
	}
	
	et::VariantBase::Pointer result;
	
	if (json_is_object(root))
	{
		c = VariantClass::Dictionary;
		result = deserializeDictionary(root);
	}
	else if (json_is_array(root))
	{
		c = VariantClass::Array;
		result = deserializeArray(root);
	}
	else
	{
		ET_FAIL("Unsupported root object type.");
	}
	
	json_decref(root);
	return result;
}

StringValue deserializeString(json_t* json)
{
	ET_ASSERT(json_is_string(json));
	return StringValue(std::string(json_string_value(json)));
}

IntegerValue deserializeInteger(json_t* json)
{
	ET_ASSERT(json_is_integer(json));
	return IntegerValue(json_integer_value(json));
}

FloatValue deserializeFloat(json_t* json)
{
	ET_ASSERT(json_is_real(json));
	return FloatValue(static_cast<float>(json_real_value(json)));
}

ArrayValue deserializeArray(json_t* json)
{
	ET_ASSERT(json_is_array(json));
	
	ArrayValue result;
	for (size_t i = 0; i < json_array_size(json); ++i)
	{
		json_t* value = json_array_get(json, i);
		if (json_is_string(value))
			result->content.push_back(deserializeString(value));
		else if (json_is_integer(value))
			result->content.push_back(deserializeInteger(value));
		else if (json_is_real(value))
			result->content.push_back(deserializeFloat(value));
		else if (json_is_array(value))
			result->content.push_back(deserializeArray(value));
		else if (json_is_object(value))
			result->content.push_back(deserializeDictionary(value));
		else if (json_is_null(value))
			result->content.push_back(Dictionary());
		else if (value)
		{
			ET_FAIL_FMT("Unsupported JSON type: %d", value->type);
		}
	}
	return result;
}

Dictionary deserializeDictionary(json_t* root)
{
	ET_ASSERT(json_is_object(root));
	
	Dictionary result;
	
	void* child = json_object_iter(root);
	while (child)
	{
		const std::string& key = json_object_iter_key(child);
		json_t* value = json_object_iter_value(child);
		
		if (json_is_string(value))
			result.setStringForKey(key, deserializeString(value));
		else if (json_is_integer(value))
			result.setIntegerForKey(key, deserializeInteger(value));
		else if (json_is_real(value))
			result.setFloatForKey(key, deserializeFloat(value));
		else if (json_is_array(value))
			result.setArrayForKey(key, deserializeArray(value));
		else if (json_is_object(value))
			result.setDictionaryForKey(key, deserializeDictionary(value));
		else if (json_is_null(value))
			result.setDictionaryForKey(key, Dictionary());
		else if (json_is_true(value))
			result.setBooleanForKey(key, 1);
		else if (json_is_false(value))
			result.setBooleanForKey(key, 0);
		else if (value != nullptr)
		{
			ET_FAIL_FMT("Unsupported JSON type: %d", value->type);
		}
		child = json_object_iter_next(root, child);
	}
	
	return result;
}

json_t* serializeArray(const ArrayValue& value)
{
	json_t* array = json_array();
	
	for (auto i : value->content)
	{
		json_t* arrayValue = serializeValue(i);
		json_array_append(array, arrayValue);
		json_decref(arrayValue);
	}

	return array;
}

json_t* serializeString(const StringValue& value)
{
	return json_string(value->content.c_str());
}

json_t* serializeInteger(const IntegerValue& value)
{
	return json_integer(value->content);
}

json_t* serializeBoolean(const BooleanValue& value)
{
	return (value->content == 0) ? json_false() : json_true();
}

json_t* serializeFloat(const FloatValue& value)
{
	return json_real(value->content);
}

json_t* serializeDictionary(const Dictionary& msg)
{
	json_t* root = json_object();
		
	for (auto& v : msg->content)
	{
		json_t* value = serializeValue(v.second);
		json_object_set(root, v.first.c_str(), value);
		json_decref(value);
	}
		
	return root;
}

json_t* serializeValue(const VariantBase::Pointer& v)
{
	json_t* value = nullptr;
	
	if (v->variantClass() == VariantClass::String)
		value = serializeString(v);
	else if (v->variantClass() == VariantClass::Integer)
		value = serializeInteger(v);
	else if (v->variantClass() == VariantClass::Boolean)
		value = serializeBoolean(v);
	else if (v->variantClass() == VariantClass::Float)
		value = serializeFloat(v);
	else if (v->variantClass() == VariantClass::Array)
		value = serializeArray(v);
	else if (v->variantClass() == VariantClass::Dictionary)
		value = serializeDictionary(v);
	else
	{
		ET_FAIL_FMT("Unknown dictionary class %d", v->variantClass());
	}
	
	return value;
}
