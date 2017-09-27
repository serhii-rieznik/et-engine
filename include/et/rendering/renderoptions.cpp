/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/json.h>
#include <et/app/application.h>
#include <et/rendering/renderoptions.h>

namespace et
{

const std::map<RenderOptions::OptionClass, std::string> RenderOptions::OptionClassNames =
{
	{ RenderOptions::OptionClass::ShadowMapping, "ShadowMapping" },
	{ RenderOptions::OptionClass::ToneMapping, "ToneMapping" },
	{ RenderOptions::OptionClass::SRGBConversion, "SRGBConversion" },
};

const std::map<std::string, RenderOptions::OptionClass> RenderOptions::OptionClassValues =
{
	{ "ShadowMapping", RenderOptions::OptionClass::ShadowMapping },
	{ "ToneMapping", RenderOptions::OptionClass::ToneMapping },
	{ "SRGBConversion", RenderOptions::OptionClass::SRGBConversion },
};

void RenderOptions::load()
{
	std::string filePath = application().resolveFileName("engine_data/config/rendering.json");
	if (!fileExists(filePath))
	{
		log::error("Failed to load rendering options. rendering.json seems to be missing or unavailable.");
		return;
	}

	VariantClass vc = VariantClass::Invalid;
	Dictionary options = json::deserialize(loadTextFile(filePath), vc);
	if (vc != VariantClass::Dictionary)
	{
		log::error("Failed to load rendering options. rendering.json has invalid content");
		return;
	}

	for (const auto& p : options->content)
	{
		auto i = OptionClassValues.find(p.first);
		if ((p.second->variantClass() == VariantClass::Dictionary) && (i != OptionClassValues.end()))
			loadOptions(i->second, p.second);
	}

	rebuildOptionsHeaderBase();
}

void RenderOptions::loadOptions(OptionClass cls, const Dictionary& obj)
{
	ArrayValue values = obj.arrayForKey("values");
	StringValue defaultValue = obj.stringForKey("default");
	StringValue currentValue = obj.stringForKey("current");

	Vector<Option>& options = _options[cls];
	options.clear();

	uint32_t index = 0;
	uint32_t defaultValueIndex = 0;
	uint32_t currentValueIndex = InvalidIndex;
	for (StringValue v : values->content)
	{
		options.emplace_back();
		Option& option = options.back();
		option.name = v->content;
		option.index = index;

		if (option.name == defaultValue->content)
			defaultValueIndex = index;
		
		if (option.name == currentValue->content)
			currentValueIndex = index;

		++index;
	}

	for (Option& option : options)
		option.defaultValue = defaultValueIndex;

	setOptionValueInternal(cls, (currentValueIndex == InvalidIndex) ? defaultValueIndex : currentValueIndex);
}

Vector<RenderOptions::OptionClass> RenderOptions::availableOptions() const
{
	Vector<RenderOptions::OptionClass> result;
	result.reserve(_options.size());
	for (const auto& p : _options)
		result.emplace_back(p.first);
	return result;
}

const RenderOptions::Option& RenderOptions::optionValue(OptionClass cls) const
{
	uint32_t valueIndex = _values.at(cls);
	return _options.at(cls).at(valueIndex);
}

void RenderOptions::setOptionValueInternal(OptionClass cls, uint32_t val)
{
	_values[cls] = val;
}

void RenderOptions::setOptionValue(OptionClass cls, uint32_t value) 
{
	ValueChangedEvent e = { cls, _values[cls], value };
	if (e.newValue != e.previousValue)
	{
		setOptionValueInternal(cls, value);
		rebuildOptionsHeader();
		optionChanged.invoke(e);
	}
}

void RenderOptions::rebuildOptionsHeaderBase()
{
	char buffer[2048] = {};
	int32_t printPosition = 0;
	
	for (const auto& p : _options)
	{
		const std::string& optionName = OptionClassNames.at(p.first);
		for (const Option& opt : p.second)
		{
			printPosition += sprintf(buffer + printPosition, "#define %s%s %d\n", optionName.c_str(), opt.name.c_str(), opt.index);
		}
	}

	_optionsHeaderBase = buffer;
	rebuildOptionsHeader();
}

void RenderOptions::rebuildOptionsHeader()
{
	char buffer[2048] = {};
	int32_t printPosition = 0;

	for (const auto& p : _values) 
	{
		const std::string& optionName = OptionClassNames.at(p.first);
		const Option& option = optionValue(p.first);
		printPosition += sprintf(buffer + printPosition, "#define %s %s%s\n", optionName.c_str(), optionName.c_str(), option.name.c_str());
	}
	
	_optionsHeader = _optionsHeaderBase + buffer;
}

}
