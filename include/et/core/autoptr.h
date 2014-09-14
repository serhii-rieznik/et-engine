/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	template <typename T>
	class AutoPtr
	{
	public: 
		AutoPtr() : _data(0) 
			{ }

		explicit AutoPtr(T* d) : _data(d)
			{ } 

		AutoPtr(AutoPtr&& p) : _data(p.extract())
			{ } 

		~AutoPtr()
		{ 
			delete _data;
			_data = 0;
		}

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
			_data = 0;
			return value;
		}

		void release()
		{
			delete _data;
			_data = 0;
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
		AutoPtr(const AutoPtr<T>&) : _data(0)
			{ }

		AutoPtr& operator = (const AutoPtr<T>&)
			{ return *this; }

	private:
		T* _data;
	};

}
