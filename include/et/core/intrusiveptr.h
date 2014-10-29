/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	#define ET_DECLARE_POINTER(T)					typedef et::IntrusivePtr<T> Pointer;
	
	ObjectFactory& sharedObjectFactory();
	
	typedef AtomicCounter Shared;

	template <typename T>
	class IntrusivePtr
	{
	public:
		IntrusivePtr() :
			_data(nullptr) { }

		IntrusivePtr(const IntrusivePtr& r) :
			_data(nullptr) { reset(r._data);  }

		IntrusivePtr(IntrusivePtr&& r) :
			_data(nullptr) { std::swap(_data, r._data); }
		
		template <typename R>
		IntrusivePtr(IntrusivePtr<R> r) :
			_data(nullptr) { reset(static_cast<T*>(r.ptr()));  }

		explicit IntrusivePtr(T* data) :
			_data(nullptr) { reset(data); }
			
		virtual ~IntrusivePtr() 
			{ reset(nullptr); }
		
#if (ET_SUPPORT_VARIADIC_TEMPLATES)
		
		template <typename ...args>
		static IntrusivePtr create(args&&...a)
			{ return IntrusivePtr<T>(sharedObjectFactory().createObject<T>(a...)); }
		
#else
#
#	error Please compile with variadic templates enabled
#
#endif
		
		T* operator *()
			{ ET_ASSERT(valid()); return _data; }

		const T* operator *() const 
			{ ET_ASSERT(valid()); return _data; }

		T* operator -> ()
			{ ET_ASSERT(valid()); return _data; }

		const T* operator -> () const 
			{ ET_ASSERT(valid()); return _data; }

		T* ptr()
			{ return _data; }

		const T* ptr() const 
			{ return _data; }

		T& reference()
			{ ET_ASSERT(valid()); return *_data; }

		const T& reference() const
			{ ET_ASSERT(valid()); return *_data; }

		bool invalid() const
			{ return _data == nullptr; }

		bool valid() const 
			{ return _data != nullptr; }

		bool operator == (const IntrusivePtr& r) const
			{ return _data == r._data; }

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
				sharedObjectFactory().deleteObject<T>(_data);
			
			_data = data;

			if (_data)
				_data->retain();
		}

	private:
		T* _data;
	};

}
