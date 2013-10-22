/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	#define ET_DECLARE_POINTER(T)					typedef et::IntrusivePtr<T> Pointer; \
	
	typedef AtomicCounter Shared;

	template <typename T>
	class IntrusivePtr
	{
	public:
		IntrusivePtr() : _data(nullptr)
			{ }

		IntrusivePtr(const IntrusivePtr& r) : _data(nullptr)
			{ reset(r._data);  }

		IntrusivePtr(IntrusivePtr&& r) : _data(nullptr)
			{ std::swap(_data, r._data); }
		
		template <typename R>
		IntrusivePtr(IntrusivePtr<R> r) : _data(nullptr)
			{ reset(static_cast<T*>(r.ptr()));  }

		explicit IntrusivePtr(T* data) : _data(nullptr)
			{ reset(data); }
			
		virtual ~IntrusivePtr() 
			{ reset(nullptr); }
		
#if (ET_SUPPORT_VARIADIC_TEMPLATES)
		template <typename ...args>
		static IntrusivePtr create(args&&...a)
			{ return IntrusivePtr<T>(new T(a...)); }
#else
		static IntrusivePtr create()
			{ return IntrusivePtr<T>(new T); }
		
#	define ET_DECL_TEMPLATE(...)	template <__VA_ARGS__> static IntrusivePtr create
#	define ET_DECL_CONSTRUCT(...)	{ return IntrusivePtr<T>(new T(__VA_ARGS__)); }
		
		ET_DECL_TEMPLATE(typename A)(A&& a)ET_DECL_CONSTRUCT(a)
		ET_DECL_TEMPLATE(typename A)(const A& a)ET_DECL_CONSTRUCT(a)
		ET_DECL_TEMPLATE(typename A, typename B)(A&& a, B&& b)ET_DECL_CONSTRUCT(a, b)
		ET_DECL_TEMPLATE(typename A, typename B)(const A& a, const B& b)ET_DECL_CONSTRUCT(a, b)
		ET_DECL_TEMPLATE(typename A, typename B, typename C)(A&& a, B&& b, C&& c)ET_DECL_CONSTRUCT(a, b, c)
		ET_DECL_TEMPLATE(typename A, typename B, typename C)(const A& a, const B& b, const C& c)ET_DECL_CONSTRUCT(a, b, c)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D)(A&& a, B&& b, C&& c, D&& d)ET_DECL_CONSTRUCT(a, b, c, d)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D)(const A& a, const B& b, const C& c, const D& d)ET_DECL_CONSTRUCT(a, b, c, d)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D, typename E)(A&& a, B&& b, C&& c, D&& d, E&& e)ET_DECL_CONSTRUCT(a, b, c, d, e)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D, typename E)(const A& a, const B& b, const C& c, const D& d, E&& e)ET_DECL_CONSTRUCT(a, b, c, d, e)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D, typename E, typename F)(A&& a, B&& b, C&& c, D&& d, E&& e, F&& f)ET_DECL_CONSTRUCT(a, b, c, d, e, f)
		ET_DECL_TEMPLATE(typename A, typename B, typename C, typename D, typename E, typename F)(const A& a, const B& b, const C& c, const D& d, E&& e, F&& f)ET_DECL_CONSTRUCT(a, b, c, d, e, f)
		
#	undef ET_DECL_CONSTRUCT
#	undef ET_DECL_TEMPLATE
		
#endif
		
		T* operator *() 
			{ return _data; }

		const T* operator *() const 
			{ return _data; }

		T* operator -> ()
			{ return _data; }

		const T* operator -> () const 
			{ return _data; }

		T* ptr()
			{ return _data; }

		const T* ptr() const 
			{ return _data; }

		T& reference()
			{ return *_data; }

		const T& reference() const
			{ return *_data; }

		bool invalid() const
			{ return _data == nullptr; }

		bool valid() const 
			{ return _data != nullptr; }

		bool operator == (const IntrusivePtr& r) const
			{ return _data == r._data; }

		bool operator == (T* tr) const
			{ return _data == tr; }

		bool operator != (const IntrusivePtr& tr) const
			{ return _data != tr._data; }

		bool operator < (const IntrusivePtr& tr) const
			{ return _data < tr._data; }
		
		AtomicCounterType referenceCount() const
			{ return _data ? _data->atomicCounterValue() : 0; }

		IntrusivePtr<T>& operator = (const IntrusivePtr<T>& r)
		{ 
			reset(r._data); 
			return *this; 
		}

		template <typename R>
		IntrusivePtr<T>& operator = (IntrusivePtr<R> r)
		{
			reset(static_cast<T*>(r.ptr()));
			return *this;
		}

		void reset(T* data) 
		{
			if (data == _data) return;

			if (_data && (_data->release() == 0))
				delete _data;
			
			_data = data;

			if (_data)
				_data->retain();
		}

	private:
		T* _data;
	};

}