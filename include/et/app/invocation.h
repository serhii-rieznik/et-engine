/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/autoptr.h>
#include <et/app/runloop.h>
#include <et/tasks/tasks.h>

namespace et
{
	class PureInvocationTarget
	{
	public:
		PureInvocationTarget() { }
		virtual ~PureInvocationTarget() { };

		virtual void invoke() = 0;
		virtual PureInvocationTarget* copy() = 0;
	};

	class PureInvocation
	{
	public:
		PureInvocation() :
			_target(nullptr) { };
		
		virtual ~PureInvocation() { };

		virtual void invoke() = 0;
		virtual void invokeInMainRunLoop(float delay) = 0;

	protected:
		AutoPtr<PureInvocationTarget> _target;
	};
	
	class InvocationTask : public Task
	{
	public:
		InvocationTask(PureInvocationTarget* invocation);
		~InvocationTask();
		
		void execute();

	private:
		PureInvocationTarget* _invocation = nullptr;
	};

	template <typename T>
	class InvocationTarget : public PureInvocationTarget
	{
	public:
		InvocationTarget(T* o, void(T::*m)()) : _object(o), _method(m) { }

		void invoke()
			{ (_object->*_method)(); }

		PureInvocationTarget* copy()
			{ return new InvocationTarget(_object, _method); }

	private:
		ET_DENY_COPY(InvocationTarget)
		
	private:
		T* _object = nullptr;
		void (T::*_method)();
	};
	
	template <typename F>
	class DirectInvocationTarget : public PureInvocationTarget
	{
	public:
		DirectInvocationTarget(F f) :
			_func(f) { }
		
		void invoke()
			{ _func(); }
		
		PureInvocationTarget* copy()
			{ return new DirectInvocationTarget<F>(_func); }
		
	private:
		ET_DENY_COPY(DirectInvocationTarget)
		
	private:
		F _func;
	};

	template <typename T, typename A1>
	class Invocation1Target : public PureInvocationTarget
	{
	public:
		Invocation1Target(T* o, void(T::*m)(A1), A1 p1) :
			_object(o), _method(m), _param(p1) { }

		void invoke()
			{ (_object->*_method)(_param); }

		void setParameter(A1 p1)
			{ _param = p1; }

		PureInvocationTarget* copy() 
			{ return new Invocation1Target(_object, _method, _param); }

	private:
		ET_DENY_COPY(Invocation1Target)

	private:
		T* _object = nullptr;
		void (T::*_method)(A1);
		A1 _param;
	};

	template <typename F, typename A1>
	class DirectInvocation1Target : public PureInvocationTarget
	{
	public:
		DirectInvocation1Target(F func, A1 p1) :
			_func(func), _param(p1) { }
		
		void invoke()
			{ _func(_param); }
		
		void setParameter(A1 p1)
			{ _param = p1; }
		
		PureInvocationTarget* copy()
			{ return new DirectInvocation1Target(_func, _param); }
		
	private:
		ET_DENY_COPY(DirectInvocation1Target)
		
	private:
		F _func;
		A1 _param;
	};
	
	template <typename T, typename A1, typename A2>
	class Invocation2Target : public PureInvocationTarget
	{
	public:
		Invocation2Target(T* o, void(T::*m)(A1, A2), A1 p1, A2 p2) : 
			_object(o), _method(m), _p1(p1), _p2(p2) { }

		void invoke()
			{ (_object->*_method)(_p1, _p2); }

		void setParameters(A1 p1, A2 p2)
		{ 
			_p1 = p1; 
			_p2 = p2;
		}

		PureInvocationTarget* copy() 
			{ return new Invocation2Target(_object, _method, _p1, _p2); }

	private:
		Invocation2Target operator = (const Invocation2Target&) 
			{ return *this; }

	private:
		T* _object = nullptr;
		void (T::*_method)(A1, A2);
		A1 _p1;
		A2 _p2;
	};

	template <typename F, typename A1, typename A2>
	class DirectInvocation2Target : public PureInvocationTarget
	{
	public:
		DirectInvocation2Target(F func, A1 p1, A2 p2) :
			_func(func), _param1(p1), _param2(p2) { }
		
		void invoke()
			{ _func(_param1, _param2); }
		
		void setParameters(A1 p1, A2 p2)
			{ _param1 = p1; _param2 = p2; }
		
		PureInvocationTarget* copy()
			{ return new DirectInvocation2Target(_func, _param1, _param2); }
		
	private:
		ET_DENY_COPY(DirectInvocation2Target)
		
	private:
		F _func;
		A1 _param1;
		A2 _param2;
	};
	
	class Invocation : public PureInvocation
	{
	public:
		void invoke();
		void invokeInMainRunLoop(float delay = 0.0f);
		void invokeInBackground(float delay = 0.0f);
		void invokeInRunLoop(RunLoop& rl, float delay = 0.0f);

		template <typename T>
		void setTarget(T* o, void(T::*m)())
			{ ET_ASSERT(o != nullptr); _target = new InvocationTarget<T>(o, m); }
		
		template <typename F>
		void setTarget(F func)
			{ _target = new DirectInvocationTarget<F>(func); }
	};

	class Invocation1 : public PureInvocation
	{
	public:
		void invoke();
		void invokeInMainRunLoop(float delay = 0.0f);
		void invokeInBackground(float delay = 0.0f);
		void invokeInRunLoop(RunLoop& rl, float delay = 0.0f);

		template <typename T, typename A1>
		void setTarget(T* o, void(T::*m)(A1), A1 param)
			{ ET_ASSERT(o != nullptr); _target = new Invocation1Target<T, A1>(o, m, param); }

		template <typename F, typename A1>
		void setTarget(F func, A1 param)
			{ _target = new DirectInvocation1Target<F, A1>(func, param); }
		
		template <typename T, typename A1>
		void setParameter(A1 p)
		{
			ET_ASSERT(_target.valid());
			(static_cast<Invocation1Target<T, A1>*>(_target.ptr()))->setParameter(p);
		}
	};

	class Invocation2 : public PureInvocation
	{
	public:
		void invoke();
		void invokeInMainRunLoop(float delay = 0.0f);
		void invokeInBackground(float delay = 0.0f);
		void invokeInRunLoop(RunLoop& rl, float delay = 0.0f);

		template <typename T, typename A1, typename A2>
		void setTarget(T* o, void(T::*m)(A1, A2), A1 p1, A2 p2)
			{ ET_ASSERT(o != nullptr); _target = new Invocation2Target<T, A1, A2>(o, m, p1, p2); }

		template <typename F, typename A1, typename A2>
		void setTarget(F func, A1 param1, A2 param2)
			{ _target = new DirectInvocation2Target<F, A1, A2>(func, param1, param2); }
		
		template <typename T, typename A1, typename A2>
		void setParameters(A1 p1, A2 p2)
			{ (static_cast<Invocation2Target<T, A1, A2>*>(_target.ptr()))->setParameters(p1, p2); }
	};

/*
 * 0
 */
	
#define ET_INVOKE_THIS_CLASS_METHOD(CLASS, METHOD)	\
		{ Invocation _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD);\
		_aInvocation.invokeInMainRunLoop(); }

#define ET_INVOKE_THIS_CLASS_METHOD_DELAYED(CLASS, METHOD, DELAY)	\
		{ Invocation _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD);\
		_aInvocation.invokeInMainRunLoop(DELAY); }

#define ET_INVOKE_THIS_CLASS_METHOD_IN_BACKGROUND(CLASS, METHOD)	\
		{ Invocation _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD);\
		_aInvocation.invokeInBackground(); }
	
#define ET_INVOKE_THIS_CLASS_METHOD_IN_BACKGROUND_DELAYED(CLASS, METHOD, DELAY)	\
		{ Invocation _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD);\
		_aInvocation.invokeInBackground(DELAY); }
	
/*
 * 1
 */
	
#define ET_INVOKE_THIS_CLASS_METHOD1(CLASS, METHOD, P1) \
		{ Invocation1 _aInvocation1; \
		_aInvocation1.setTarget(this, &CLASS::METHOD, P1); \
		_aInvocation1.invokeInMainRunLoop(); }

#define ET_INVOKE_THIS_CLASS_METHOD1_DELAYED(CLASS, METHOD, P1, DELAY) \
		{ Invocation1 _aInvocation1; \
		_aInvocation1.setTarget(this, &CLASS::METHOD, P1); \
		_aInvocation1.invokeInMainRunLoop(DELAY); }
	
#define ET_INVOKE_THIS_CLASS_METHOD1_IN_BACKGROUND(CLASS, METHOD, P1) \
		{ Invocation1 _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD, P1);\
		_aInvocation.invokeInBackground(); }

#define ET_INVOKE_THIS_CLASS_METHOD1_IN_BACKGROUND_DELAYED(CLASS, METHOD, P1, DELAY) \
		{ Invocation1 _aInvocation;\
		_aInvocation.setTarget(this, &CLASS::METHOD, P1);\
		_aInvocation.invokeInBackground(DELAY); }

/*
 * 2
 */

#define ET_INVOKE_THIS_CLASS_METHOD2(CLASS, METHOD, P1, P2)	\
		{ Invocation2 _aInvocation2; \
		_aInvocation2.setTarget(this, &CLASS::METHOD, P1, P2); \
		_aInvocation2.invokeInMainRunLoop(); }

#define ET_INVOKE_THIS_CLASS_METHOD2_DELAYED(CLASS, METHOD, P1, P2, DELAY) \
		{ Invocation2 _aInvocation2; \
		_aInvocation2.setTarget(this, &CLASS::METHOD, P1, P2); \
		_aInvocation2.invokeInMainRunLoop(DELAY); }
}
