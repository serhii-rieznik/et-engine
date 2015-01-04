/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	enum MaterialParameters
	{
		MaterialParameter_Undefined,
		
		MaterialParameter_AmbientColor,
		MaterialParameter_DiffuseColor,
		MaterialParameter_SpecularColor,
		MaterialParameter_EmissiveColor,
		
		MaterialParameter_AmbientMap,
		MaterialParameter_DiffuseMap,
		MaterialParameter_SpecularMap,
		MaterialParameter_EmissiveMap,
		MaterialParameter_NormalMap,
		MaterialParameter_BumpMap,
		MaterialParameter_ReflectionMap,
		MaterialParameter_TransparencyMap,
		
		MaterialParameter_AmbientFactor,
		MaterialParameter_DiffuseFactor,
		MaterialParameter_SpecularFactor,
		MaterialParameter_BumpFactor,
		MaterialParameter_ReflectionFactor,
		
		MaterialParameter_Roughness,
		MaterialParameter_Transparency,
		MaterialParameter_ShadingModel,

		MaterialParameter_TransparentColor,

		MaterialParameter_max,
		MaterialParameter_User = 0xffff
	};
	
	template <typename T>
	struct DefaultMaterialEntryBase
	{
		T value;
		size_t set;
		
	public:
		DefaultMaterialEntryBase& operator = (const DefaultMaterialEntryBase& r)
		{
			this->value = r.value;
			this->set = r.set;
			return *this;
		}
		
	protected:
		DefaultMaterialEntryBase() :
			value(0), set(0) { }
		
		DefaultMaterialEntryBase(const T& v) :
			value(v), set(1) { }
	};
	
	template <typename T>
	struct DefaultMaterialEntry : public DefaultMaterialEntryBase<T>
	{
		DefaultMaterialEntry& operator = (T r)
		{
			this->value = r;
			this->set = r != 0;
			return *this;
		}
	};
		
	template <>
	struct DefaultMaterialEntry<std::string> : public DefaultMaterialEntryBase<std::string>
	{
		DefaultMaterialEntry() :
			DefaultMaterialEntryBase(emptyString) { }
		
		DefaultMaterialEntry& operator = (const std::string& r)
		{
			this->value = r;
			this->set = !r.empty();
			return *this;
		}
	};

	template <>
	struct DefaultMaterialEntry<vec4> : public DefaultMaterialEntryBase<vec4>
	{
		DefaultMaterialEntry& operator = (const vec4& r)
		{
			this->value = r;
			this->set = r.dotSelf() > 0.0f;
			return *this;
		}
	};
	
	template <>
	struct DefaultMaterialEntry<Texture::Pointer> : public DefaultMaterialEntryBase<Texture::Pointer>
	{
		DefaultMaterialEntry& operator = (const Texture::Pointer& r)
		{
			this->value = r;
			this->set = r.valid();
			return *this;
		}
	};
			
	template <typename T>
	struct DefaultParameters
	{
		DefaultMaterialEntry<T> values[MaterialParameter_max];
		
		DefaultMaterialEntry<T>& operator[] (size_t i)
		{
			ET_ASSERT(i < MaterialParameter_max);
			return values[i];
		}
		
		const DefaultMaterialEntry<T>& operator[] (size_t i) const
		{
			ET_ASSERT(i < MaterialParameter_max);
			return values[i];
		}
		
		DefaultParameters& operator = (const DefaultParameters& r)
		{
			for (size_t i = 0; i < MaterialParameter_max; ++i)
				values[i] = r.values[i];
			return *this;
		}
	};
		
	typedef std::map<size_t, int> CustomIntParameters;
	typedef std::map<size_t, float> CustomFloatParameters;
	typedef std::map<size_t, vec4> CustomVectorParameters;
	typedef std::map<size_t, Texture::Pointer> CustomTextureParameters;
	typedef std::map<size_t, std::string> CustomStringParameters;
	
	typedef DefaultParameters<int> DefaultIntParameters;
	typedef DefaultParameters<float> DefaultFloatParameters;
	typedef DefaultParameters<vec4> DefaultVectorParameters;
	typedef DefaultParameters<Texture::Pointer> DefaultTextureParameters;
	typedef DefaultParameters<std::string> DefaultStringParameters;
}

