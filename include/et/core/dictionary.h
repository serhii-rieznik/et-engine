/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/intrusiveptr.h>

namespace et
{
    enum class VariantClass : int32_t
	{
		Invalid = -1,
		Float,
		Integer,
		Boolean,
		String,
		Array,
		Dictionary,
	};
	
	class VariantBase : public Shared
	{
	public:
		ET_DECLARE_POINTER(VariantBase)
						   
 	public:
		virtual ~VariantBase() { }
		virtual VariantClass variantClass() const = 0;
	};
	
	template <typename T, VariantClass C>
	class Variant : public VariantBase
	{
	public:
		using Pointer =  et::IntrusivePtr<Variant<T, C>>;
		
	public:
		T content;
		
	public:
		Variant() :
			content(T()) { }
		
		Variant(const T& r) :
			content(r) { }

        VariantClass variantClass() const override
			{ return C; }
	};
	
	typedef std::function<void(VariantBase::Pointer)> VariantCallbackFunction;
		
	template <typename T, VariantClass C>
	class VariantPointer : public Variant<T, C>::Pointer
	{
	public:
		using ValueType = T;
		using PointerType = typename Variant<T, C>::Pointer;
		
	public:
		VariantPointer() :
			Variant<T, C>::Pointer(etCreateObject<Variant<T, C>>()) { }
		
		VariantPointer(const T& r) :
			Variant<T, C>::Pointer(etCreateObject<Variant<T, C>>(r)) { }

		VariantPointer(const typename Variant<T, C>::Pointer& p) :
			Variant<T, C>::Pointer(p) { }

		VariantPointer(Variant<T, C>* p) :
			Variant<T, C>::Pointer(p) { }

		VariantPointer(VariantBase::Pointer p) :
			Variant<T, C>::Pointer(p) { }
			
		virtual void performRecursive(VariantCallbackFunction func)
			{ func(*this); }

		virtual VariantPointer<T, C> duplicate() const
			{ return VariantPointer<T, C>(this->reference().content); }
	};
	
	typedef VariantPointer<float, VariantClass::Float> FloatValue;
	typedef VariantPointer<int64_t, VariantClass::Integer> IntegerValue;
	typedef VariantPointer<int, VariantClass::Boolean> BooleanValue;
	
	class StringValue : public VariantPointer<std::string, VariantClass::String>
	{
	public:
		StringValue() :
			VariantPointer<std::string, VariantClass::String>() { }
		
		StringValue(const std::string& r) :
			VariantPointer<std::string, VariantClass::String>(r) { }

		StringValue(const char* r) :
			VariantPointer<std::string, VariantClass::String>(std::string(r)) { }
		
		StringValue(const Variant<std::string, VariantClass::String>::Pointer& p) :
			VariantPointer<std::string, VariantClass::String>(p) { }
		
		StringValue(Variant<std::string, VariantClass::String>* p) :
			VariantPointer<std::string, VariantClass::String>(p) { }
		
		StringValue(VariantBase::Pointer p) :
			VariantPointer<std::string, VariantClass::String>(p) { }
		
		size_t size() const
			{ return reference().content.size(); }

		bool empty() const
			{ return reference().content.empty(); }
	};
	
	class ArrayValue : public VariantPointer<Vector<VariantBase::Pointer>, VariantClass::Array>
	{
	public:
		ArrayValue() :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>( ) { }
		
		ArrayValue(const VariantPointer<ArrayValue::ValueType, VariantClass::Array>& r) :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>(r) { }
		
		ArrayValue(const ArrayValue& r) :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>(r) { }
		
		ArrayValue(ArrayValue&& r) :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>(r) {	}
		
		ArrayValue(VariantBase::Pointer p) :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>(p) { }
		
		ArrayValue(const VariantPointer::ValueType& c) :
			VariantPointer<ArrayValue::ValueType, VariantClass::Array>( ) { reference().content = c; }
		
		ArrayValue& operator = (const ArrayValue& r)
		{
			PointerType::operator = (r);
			return *this;
		}
		
		inline void performRecursive(VariantCallbackFunction func) override;
					
	public:
		void printContent() const;
		VariantPointer<ValueType, VariantClass::Array> duplicate() const override;
	};
	
	class Dictionary : public VariantPointer<UnorderedMap<std::string, VariantBase::Pointer>, VariantClass::Dictionary>
	{
	public:
		Dictionary() :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>( ) { }
		
		Dictionary(const VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>& r) :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>(r) { }

		Dictionary(const Dictionary& r) :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>(r) { }

		Dictionary(Dictionary&& r) :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>(r) {	}
		
		Dictionary(VariantBase::Pointer p) :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>(p) { }
		
		Dictionary(const VariantPointer::ValueType& c) :
			VariantPointer<Dictionary::ValueType, VariantClass::Dictionary>( ) { reference().content = c; }
		
		Dictionary(const std::string& json);
		
		Dictionary& operator = (const Dictionary& r)
		{
			PointerType::operator = (r);
			return *this;
		}
		
	public:
		void setObjectForKey(const std::string& key, VariantBase::Pointer value)
			{ setValueForKey<VariantBase::Pointer>(key, value); }

		void setStringForKey(const std::string& key, StringValue value)
			{ setValueForKey<StringValue>(key, value); }

		void setIntegerForKey(const std::string& key, IntegerValue value)
			{ setValueForKey<IntegerValue>(key, value); }

		void setBooleanForKey(const std::string& key, BooleanValue value)
			{ setValueForKey<BooleanValue>(key, value); }
		
		void setFloatForKey(const std::string& key, FloatValue value)
			{ setValueForKey<FloatValue>(key, value); }
		
		void setArrayForKey(const std::string& key, ArrayValue value)
			{ setValueForKey<ArrayValue>(key, value); }

		void setDictionaryForKey(const std::string& key, Dictionary value)
			{ setValueForKey<Dictionary>(key, value); }

		void setFloatForKeyPath(const StringList& keyPath, FloatValue value)
			{ setValueForKeyPath<FloatValue, VariantClass::Float>(keyPath, value); }
		
		inline void performRecursive(VariantCallbackFunction func) override;
		
	public:
		BooleanValue boolForKey(const std::string& key, BooleanValue def = BooleanValue()) const
			{ return valueForKey<BooleanValue::ValueType, VariantClass::Boolean>(key, def); }
		BooleanValue boolForKeyPath(const StringList& key, BooleanValue def = BooleanValue()) const
			{ return valueForKeyPath<BooleanValue::ValueType, VariantClass::Boolean>(key, def); }
		
		IntegerValue integerForKey(const std::string& key, IntegerValue def = IntegerValue()) const
			{ return valueForKey<IntegerValue::ValueType, VariantClass::Integer>(key, def); }
		IntegerValue integerForKeyPath(const StringList& key, IntegerValue def = IntegerValue()) const
			{ return valueForKeyPath<IntegerValue::ValueType, VariantClass::Integer>(key, def); }

		FloatValue floatForKey(const std::string& key, FloatValue def = FloatValue()) const
			{ return valueForKey<FloatValue::ValueType, VariantClass::Float>(key, def); }
		FloatValue floatForKeyPath(const StringList& key, FloatValue def = FloatValue()) const
			{ return valueForKeyPath<FloatValue::ValueType, VariantClass::Float>(key, def); }
		
		StringValue stringForKey(const std::string& key, StringValue def = StringValue()) const
			{ return valueForKey<StringValue::ValueType, VariantClass::String>(key, def); }
		StringValue stringForKeyPath(const StringList& key, StringValue def = StringValue()) const
			{ return valueForKeyPath<StringValue::ValueType, VariantClass::String>(key, def); }
		
		ArrayValue arrayForKey(const std::string& key, ArrayValue def = ArrayValue()) const
			{ return valueForKey<ArrayValue::ValueType, VariantClass::Array>(key, def); }
		ArrayValue arrayForKeyPath(const StringList& key, ArrayValue def = ArrayValue()) const
			{ return valueForKeyPath<ArrayValue::ValueType, VariantClass::Array>(key, def); }
		
		Dictionary dictionaryForKey(const std::string& key, Dictionary def = Dictionary()) const
			{ return Dictionary(valueForKey<Dictionary::ValueType, VariantClass::Dictionary>(key, def)); }
		Dictionary dictionaryForKeyPath(const StringList& key, Dictionary def = Dictionary()) const
			{ return Dictionary(valueForKeyPath<Dictionary::ValueType, VariantClass::Dictionary>(key, def)); }
		
	public:
		bool loadFromJson(const std::string&);
		
		std::string storeToJson() const;
		
		void removeObjectForKey(const std::string& key)
			{ reference().content.erase(key); }
		
		bool hasKey(const std::string&) const;
		
		VariantClass VariantClassForKey(const std::string&) const;
		
		VariantBase::Pointer objectForKey(const std::string& key) const;
		VariantBase::Pointer objectForKeyPath(const StringList& key) const;
		
		bool empty() const
			{ return reference().content.empty(); }
		
		StringList allKeyPaths();

		VariantPointer<ValueType, VariantClass::Dictionary> duplicate() const override;
				
	public:
		void printContent() const;

	private:
		bool valueForKeyPathIsClassOf(const StringList& key, VariantClass) const;
				
		VariantBase::Pointer baseValueForKeyPathInHolder(const StringList& key,
			VariantBase::Pointer holder) const;

		void addKeyPathsFromHolder(VariantBase::Pointer holder, const std::string& baseKeyPath,
			StringList& keyPaths) const;
		
		template <typename T>
		void setValueForKey(const std::string& key, const T& value)
			{ this->reference().content[key] = value; }

		template <typename T, VariantClass C>
		void setValueForKeyPath(const StringList& keyPath, const T& value)
		{
			auto v = objectForKeyPath(keyPath);
			if (v.invalid() || (v->variantClass() != C)) return;
			T(v)->content = value->content;
		}

		template <typename T, VariantClass C>
		VariantPointer<T, C> valueForKey(const std::string& key, VariantPointer<T, C> def) const
		{
			auto i = objectForKey(key);
			return (i.invalid() || (i->variantClass() != C)) ? def : VariantPointer<T, C>(i);
		}
		
		template <typename T, VariantClass C>
		VariantPointer<T, C> valueForKeyPath(const StringList& key, VariantPointer<T, C> def) const
		{
			auto i = objectForKeyPath(key);
			return (i.invalid() || (i->variantClass() != C)) ? def : VariantPointer<T, C>(i);
		}
	};
}

