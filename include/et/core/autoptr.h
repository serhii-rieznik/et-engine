/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	ObjectFactory& sharedObjectFactory();
	
	template <typename T>
	class AutoPtr
	{
	public: 
		AutoPtr() :
			_data(nullptr) { }

		explicit AutoPtr(T* d) :
			_data(d) { }

		AutoPtr(AutoPtr&& p) :
			_data(p.extract()) { }

		~AutoPtr()
			{ release(); }

		AutoPtr& operator = (T* p)
		{
			release();
			_data = p;
			return *this;
		}

		char* binary()
			{ return reinterpret_cast<char*>(_data); }

		const char* binary() const
			{ return reinterpret_cast<const char*>(_data); }

		T* ptr()
			{ return _data; }

		const T* ptr() const 
			{ return _data; }

		T& reference()
			{ return *_data; }

		const T& reference() const
			{ return *_data; }

		T* extract()
		{
			T* value = _data;
			_data = nullptr;
			return value;
		}

		void release()
		{
			sharedObjectFactory().deleteObject(_data);
			_data = nullptr;
		}

		T* operator -> ()
			{ return _data; }

		const T* operator -> () const
			{ return _data; }

		bool invalid() const
			{ return _data == nullptr; }

		bool valid() const 
			{ return _data != nullptr; }

	private:
		AutoPtr(const AutoPtr<T>&) = delete;
		AutoPtr& operator = (const AutoPtr<T>&) = delete;

	private:
		T* _data = nullptr;
	};

}
