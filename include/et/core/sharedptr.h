/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/threading/atomiccounter.h>

namespace et
{
	template <typename T> class SharedPtr;
	template <typename T> class WeakPtr;

	struct SharedReferenceCounter
	{
		AtomicCounter strong;
		AtomicCounter weak;
	};

	template <typename T>
	class SharedPtr
	{
	public:
		SharedPtr() : _data(nullptr), _referenceCounter(nullptr)
			{ };

		virtual ~SharedPtr()
			{ reset(nullptr, nullptr); }

		explicit SharedPtr(T* p) : _data(nullptr), referenceCount(nullptr)
			{ reset(p, nullptr); };

		SharedPtr(const SharedPtr<T>& p) : _data(nullptr), referenceCount(nullptr)
			{ reset(p._data, p._referenceCounter); };

		template <typename R>
		SharedPtr(const SharedPtr<R>& p) : _data(nullptr), _referenceCounter(nullptr)
			{ reset(p.ptr(), p.refCount()); };

		SharedPtr& operator = (const SharedPtr& r)
		{ 
			reset(r._data, r._referenceCounter);
			return *this;
		}

		SharedPtr& operator = (T* d)
		{ 
			reset(d, nullptr);
			return *this;
		}

		bool valid() const
			{ return _data != nullptr; }

		T* ptr()
			{ return _data; }

		const T* ptr() const
			{ return _data; }

		bool invalid() const
			{ return !valid(); }

		T* operator -> () 
			{ return ptr(); }

		const T* operator -> () const
			{ return ptr(); }

		T* operator *() 
			{ return ptr(); }

		const T* operator *() const 
			{ return ptr(); }

		bool operator == (const SharedPtr& r) const
			{ return _data == r._data; }

		bool operator == (T* p) const
			{ return _data == p; }

		bool operator != (const SharedPtr& r) const
			{ return _data != r._data; }

		AtomicCounterType referenceCount() const
			{ return _referenceCounter ? _referenceCounter->strong.atomicCounterValue() : 0; }

		SharedReferenceCounter* referenceCounter()
			{ return _referenceCounter; }

	private:

		void reset(T* data, SharedReferenceCounter* r)
		{
			if (data == _data) return;

			if (_referenceCounter && (_referenceCounter->strong.release() == 0))
			{
				delete _data;
				if (_referenceCounter->weak.atomicCounterValue() == 0)
					delete _referenceCounter;
			}

			_data = data;
			_referenceCounter = r;

			if (_data && !_referenceCounter)
				_referenceCounter = new SharedReferenceCounter;

			if (_referenceCounter)
				_referenceCounter->strong.retain();
		}

	private:
		T* _data;
		SharedReferenceCounter* _referenceCounter;

		friend class WeakPtr<T>;
	};

	template <typename T>
	class WeakPtr
	{
	public:
		WeakPtr() : _data(nullptr), _referenceCounter(nullptr)
			{ };

		virtual ~WeakPtr()
			{ reset(nullptr, nullptr); }

		explicit WeakPtr(T* p) : _data(nullptr), _referenceCounter(nullptr)
			{ reset(p, nullptr); };

		WeakPtr(const WeakPtr<T>& p) : _data(nullptr), _referenceCounter(nullptr)
			{ reset(p._data, p._referenceCounter); };

		WeakPtr(const SharedPtr<T>& p) : _data(nullptr), _referenceCounter(nullptr)
			{ reset(p._data, p._referenceCounter); };

		WeakPtr& operator = (const WeakPtr& r)
		{ 
			reset(r._data, r._referenceCounter);
			return *this;
		}

		WeakPtr& operator = (const SharedPtr<T>& r)
		{ 
			reset(r._data, r._referenceCounter);
			return *this;
		}

		WeakPtr& operator = (T* d)
		{ 
			reset(d, nullptr);
			return *this;
		}

		bool valid() const
			{ return _referenceCounter && (_referenceCounter->strong.atomicCounterValue() > 0); }

		T* ptr()
			{ return valid() ? _data : nullptr; }

		const T* ptr() const
			{ return valid() ? _data : nullptr; }

		bool invalid() const
			{ return !valid(); }

		T* operator -> () 
			{ return ptr(); }

		const T* operator -> () const
			{ return ptr(); }

		T* operator *() 
			{ return ptr(); }

		const T* operator *() const 
			{ return ptr(); }

		bool operator == (const WeakPtr& r) const
			{ return _data == r._data; }

		bool operator != (const WeakPtr& r) const
			{ return _data != r._data; }

		AtomicCounterType referenceCount() const
			{ return _referenceCounter ? _referenceCounter->strong.atomicCounterValue() : 0; }

	private:

		void reset(T* data, SharedReferenceCounter* r)
		{
			if (data == _data) return;

			if (_referenceCounter && (_referenceCounter->weak.release() == 0))
				delete _referenceCounter;

			_data = data;
			_referenceCounter = r;

			if (_data && !_referenceCounter)
				_referenceCounter = new SharedReferenceCounter;

			if (_referenceCounter)
				_referenceCounter->weak.retain();
		}

	private:
		T* _data;
		SharedReferenceCounter* referenceCount;
	};
}