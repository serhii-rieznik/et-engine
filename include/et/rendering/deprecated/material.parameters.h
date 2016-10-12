/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	enum class MaterialParameter : uint32_t
	{
		AmbientMap,
		DiffuseMap,
		SpecularMap,
		EmissiveMap,
		NormalMap,
		BumpMap,
		ReflectionMap,
		OpacityMap,
		
		AmbientColor,
		DiffuseColor,
		SpecularColor,
		EmissiveColor,
		
		AmbientFactor,
		DiffuseFactor,
		SpecularFactor,
		EmissiveFactor,
		MaterialParameter::NormalTextureScale,
		
		Transparency,
		Roughness,
		
		max
	};
	
	enum : uint32_t
	{
		MaterialParameter_max = static_cast<uint32_t>(MaterialParameter::max)
	};
	
	template <typename T>
	struct DefaultMaterialEntry
	{
		DefaultMaterialEntry() = default;
		
		DefaultMaterialEntry(const T& v) :
			value(v), set(1) { }
		
		DefaultMaterialEntry& operator = (const DefaultMaterialEntry& r)
		{
			this->value = r.value;
			this->set = r.set;
			return *this;
		}
		
		void clear()
		{
			value = T();
			set = 0;
		}
		
	public:
		T value = T();
		uint32_t set = 0;
	};
	
	template <typename T>
	struct ParameterSet
	{
		DefaultMaterialEntry<T> values[MaterialParameter_max];
		
		DefaultMaterialEntry<T>& operator [] (MaterialParameter i)
		{
			ET_ASSERT(i < MaterialParameter::max);
			return values[static_cast<uint32_t>(i)];
		}
		
		const DefaultMaterialEntry<T>& operator [] (MaterialParameter i) const
		{
			ET_ASSERT(i < MaterialParameter::max);
			return values[static_cast<uint32_t>(i)];
		}
		
		DefaultMaterialEntry<T>* begin()
			{ return values; }
		
		DefaultMaterialEntry<T>* end()
			{ return begin() + MaterialParameter_max; }
		
		const DefaultMaterialEntry<T>* begin() const
			{ return values; }
		
		const DefaultMaterialEntry<T>* end() const
			{ return begin() + MaterialParameter_max; }
	};
}

