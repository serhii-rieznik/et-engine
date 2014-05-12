/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/invocation.h>

namespace et
{

#	define ET_DECLARE_EVENT0(name)									et::Event0 name;
#	define ET_DECLARE_EVENT1(name, argtype)							et::Event1<argtype> name;
#	define ET_DECLARE_EVENT2(name, arg1type, arg2type)				et::Event2<arg1type, arg2type> name;
	
#	define ET_CONNECT_EVENT(name, methodName)						name.connect(this, &methodName);
	
#	define ET_CONNECT_EVENT_TO_OBJECT(name, object, methodName)		name.connect(object, &methodName);

	class Event;
	
	class EventReceiver
	{
	public:
		virtual ~EventReceiver();
		
		void eventConnected(Event* e);
		void eventDisconnected(Event* e);

	private:
		std::vector<Event*> _events;
	};

	class Event
	{
	public:
		virtual ~Event() { }
		virtual void receiverDisconnected(EventReceiver* receiver) = 0;
	};
	
	class EventConnectionBase
	{
	public:
		EventConnectionBase() : 
		  _removed(false) { }

		virtual ~EventConnectionBase() { }

		bool removed() const
			{ return _removed; }

		void remove()
			{ _removed = true; }

		void setRemoved(bool value)
			{ _removed = value; }

		virtual EventReceiver* receiver() 
			{ return nullptr; }
		
		virtual bool hasConnections()
			{ return false; }

	private:
		bool _removed;
	};

	/*
	* Event 0
	*/

	class Event0ConnectionBase : public EventConnectionBase
	{
	public:
		virtual void invoke() = 0;
		virtual void invokeInMainRunLoop(float delay) = 0;
	};

	template <typename RecevierType>
	class Event0Connection : public Event0ConnectionBase
	{
	public:
		Event0Connection(RecevierType* receiver, void(RecevierType::*func)());

		EventReceiver* receiver()
			{ return _receiver; }

		void invoke()
			{ (_receiver->*_receiverMethod)(); }

		void invokeInMainRunLoop(float delay = 0.0f)
		{
			Invocation i;
			i.setTarget(_receiver, _receiverMethod);
			i.invokeInMainRunLoop(delay);
		}

		void invokeInBackground(float delay = 0.0f)
		{
			Invocation i;
			i.setTarget(_receiver, _receiverMethod);
			i.invokeInBackground(delay);
		}
		
	private:
		void (RecevierType::*_receiverMethod)();
		RecevierType* _receiver;
	};
	
	template <typename C>
	class Event0DirectConnection : public Event0ConnectionBase
	{
	public:
		Event0DirectConnection(C func) :
			_func(func) { }
		
		EventReceiver* receiver()
			{ return nullptr; }
		
		void invoke()
			{ _func(); }
		
		void invokeInMainRunLoop(float delay = 0.0f)
		{
			Invocation i;
			i.setTarget(_func);
			i.invokeInMainRunLoop(delay);
		}
		
		void invokeInBackground(float delay = 0.0f)
		{
			Invocation i;
			i.setTarget(_func);
			i.invokeInBackground(delay);
		}
		
	private:
		C _func;
	};

	class Event0 : public Event, public Event0ConnectionBase
	{
	public:
		Event0();
		~Event0();

		template <typename R>
		void connect(R* receiver, void (R::*receiverMethod)());
		
		template <typename F>
		void connect(F func);
		
		template <typename R>
		void disconnect(R* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke();
		void invokeInMainRunLoop(float delay = 0.0f);
		
		bool hasConnections()
			{ return !_connections.empty(); }
		
	private:
		EventReceiver* receiver() 
			{ return 0; }

		void cleanup();

	private:
		typedef std::vector<Event0ConnectionBase*> ConnectionList;
		ConnectionList _connections; 
		bool _invoking;
	};

	/*
	* Event 1
	*/

	template <typename ArgType>
	class Event1ConnectionBase : public EventConnectionBase
	{
	public:
		virtual void invoke(ArgType arg) = 0;
		virtual void invokeInMainRunLoop(ArgType arg, float delay) = 0;
	};

	template <typename ReceiverType, typename ArgType>
	class Event1Connection : public Event1ConnectionBase<ArgType>
	{
	public:
		Event1Connection(ReceiverType* receiver, void(ReceiverType::*func)(ArgType));

		EventReceiver* receiver()
			{ return _receiver; }

		void invoke(ArgType arg) 
			{ (_receiver->*_receiverMethod)(arg); }

		void invokeInMainRunLoop(ArgType arg, float delay)
		{
			Invocation1 i;
			i.setTarget<ReceiverType, ArgType>(_receiver, _receiverMethod, arg);
			i.invokeInMainRunLoop(delay);
		}
		
		void invokeInBackground(ArgType arg, float delay)
		{
			Invocation1 i;
			i.setTarget<ReceiverType, ArgType>(_receiver, _receiverMethod, arg);
			i.invokeInBackground(delay);
		}

	private:
		void (ReceiverType::*_receiverMethod)(ArgType);
		ReceiverType* _receiver;
	};

	template <typename F, typename ArgType>
	class Event1DirectConnection : public Event1ConnectionBase<ArgType>
	{
	public:
		Event1DirectConnection(F f) :
			_func(f) { };
		
		EventReceiver* receiver()
			{ return nullptr; }
		
		void invoke(ArgType arg)
			{ _func(arg); }
		
		void invokeInMainRunLoop(ArgType arg, float delay)
		{
			Invocation1 i;
			i.setTarget<F, ArgType>(_func, arg);
			i.invokeInMainRunLoop(delay);
		}
		
		void invokeInBackground(ArgType arg, float delay)
		{
			Invocation1 i;
			i.setTarget<F, ArgType>(_func, arg);
			i.invokeInBackground(delay);
		}
		
	private:
		F _func;
	};
	
	template <typename ArgType>
	class Event1 : public Event, public Event1ConnectionBase<ArgType>
	{
	public:
		Event1();
		~Event1();

		template <typename ReceiverType>
		void connect(ReceiverType* receiver, void (ReceiverType::*receiverMethod)(ArgType));
		
		template <typename F>
		void connect(F);

		template <typename ReceiverType>
		void disconnect(ReceiverType* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke(ArgType arg);
		void invokeInMainRunLoop(ArgType arg, float delay = 0.0f);
		
		bool hasConnections()
			{ return !_connections.empty(); }
		
	private:
		void cleanup();

		EventReceiver* receiver() 
			{ return nullptr; }

	private:
		typedef std::vector< Event1ConnectionBase<ArgType>* > ConnectionList;
		ConnectionList _connections;
		bool _invoking;
	};

	/*
	* Event 1
	*/

	template <typename Arg1Type, typename Arg2Type>
	class Event2ConnectionBase : public EventConnectionBase
	{
	public:
		virtual void invoke(Arg1Type a1, Arg2Type a2) = 0;
		virtual void invokeInMainRunLoop(Arg1Type a1, Arg2Type a2, float delay) = 0;
	};

	template <typename ReceiverType, typename Arg1Type, typename Arg2Type>
	class Event2Connection : public Event2ConnectionBase<Arg1Type, Arg2Type>
	{
	public:
		Event2Connection(ReceiverType* receiver, void(ReceiverType::*func)(Arg1Type, Arg2Type));

		EventReceiver* receiver()
			{ return _receiver; }

		void invoke(Arg1Type a1, Arg2Type a2) 
			{ (_receiver->*_receiverMethod)(a1, a2); }
		
		void invokeInMainRunLoop(Arg1Type a1, Arg2Type a2, float delay)
		{
			Invocation2 i;
			i.setTarget<ReceiverType, Arg1Type, Arg2Type>(_receiver, _receiverMethod, a1, a2);
			i.invokeInMainRunLoop(delay);
		}
		
		void invokeInBackground(Arg1Type a1, Arg2Type a2, float delay)
		{
			Invocation2 i;
			i.setTarget<ReceiverType, Arg1Type, Arg2Type>(_receiver, _receiverMethod, a1, a2);
			i.invokeInBackground(delay);
		}

	private:
		void (ReceiverType::*_receiverMethod)(Arg1Type, Arg2Type);
		ReceiverType* _receiver;
	};
	
	template <typename F, typename ArgType1, typename ArgType2>
	class Event2DirectConnection : public Event2ConnectionBase<ArgType1, ArgType2>
	{
	public:
		Event2DirectConnection(F f) :
			_func(f) { };
		
		EventReceiver* receiver()
			{ return nullptr; }
		
		void invoke(ArgType1 arg1, ArgType2 arg2)
			{ _func(arg1, arg2); }
		
		void invokeInMainRunLoop(ArgType1 arg1, ArgType2 arg2, float delay)
		{
			Invocation2 i;
			i.setTarget<F, ArgType1, ArgType2>(_func, arg1, arg2);
			i.invokeInMainRunLoop(delay);
		}
		
		void invokeInBackground(ArgType1 arg1, ArgType2 arg2, float delay)
		{
			Invocation2 i;
			i.setTarget<F, ArgType1, ArgType2>(_func, arg1, arg2);
			i.invokeInBackground(delay);
		}
		
	private:
		F _func;
	};

	template <typename Arg1Type, typename Arg2Type>
	class Event2 : public Event, public Event2ConnectionBase<Arg1Type, Arg2Type>
	{
	public:
		Event2();
		~Event2();

		template <typename ReceiverType>
		void connect(ReceiverType* receiver, void (ReceiverType::*receiverMethod)(Arg1Type, Arg2Type));

		template <typename F>
		void connect(F);
		
		template <typename ReceiverType>
		void disconnect(ReceiverType* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke(Arg1Type a1, Arg2Type a2);
		void invokeInMainRunLoop(Arg1Type a1, Arg2Type a2, float delay = 0.0f);

		bool hasConnections()
			{ return !_connections.empty(); }
		
	private:
		EventReceiver* receiver()
			{ return nullptr; }

	private:
		typedef std::vector<Event2ConnectionBase<Arg1Type, Arg2Type>*> ConnectionList;
		ConnectionList _connections; 
		bool _invoking;
	};

#include <et/app/events.inl.h>

}
