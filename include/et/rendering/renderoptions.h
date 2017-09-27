/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/app/events.h>

namespace et
{
class RenderOptions
{
public:
	enum class OptionClass : uint32_t {
		Unspecifed,
		ShadowMapping,
		ToneMapping,
		SRGBConversion,
	};

	static const std::map<OptionClass, std::string> OptionClassNames;
	static const std::map<std::string, OptionClass> OptionClassValues;

	struct Option
	{
		std::string name;
		uint32_t index = InvalidIndex;
		uint32_t defaultValue = InvalidIndex;
	};

	struct ValueChangedEvent
	{
		OptionClass cls = OptionClass::Unspecifed;
		uint32_t previousValue = InvalidIndex;
		uint32_t newValue = InvalidIndex;
	};

public:
	void load();

	Vector<OptionClass> availableOptions() const;
	const Vector<Option>& availableOptions(OptionClass) const;
	
	const Option& optionValue(OptionClass) const;
	void setOptionValue(OptionClass, uint32_t);

	const std::string& optionsHeader() const;

	ET_DECLARE_EVENT1(optionChanged, ValueChangedEvent);

private:
	void setOptionValueInternal(OptionClass, uint32_t);
	void loadOptions(OptionClass, const Dictionary&);
	void rebuildOptionsHeaderBase();
	void rebuildOptionsHeader();

private:
	Map<OptionClass, Vector<Option>> _options;
	Map<OptionClass, uint32_t> _values;
	Option _defaultOption;
	std::string _optionsHeaderBase;
	std::string _optionsHeader;
};

inline const Vector<RenderOptions::Option>& RenderOptions::availableOptions(OptionClass cls) const {
	return _options.at(cls);
}

inline const std::string& RenderOptions::optionsHeader() const {
	return _optionsHeader;
}

}
