/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <algorithm>
#include <et/app/invocation.h>

namespace et
{

#	define ET_DECLARE_EVENT0(name)									et::Event0 name;
#	define ET_DECLARE_EVENT1(name, argtype)							et::Event1<argtype> name;
#	define ET_DECLARE_EVENT2(name, arg1type, arg2type)				et::Event2<arg1type, arg2type> name;
#	define ET_CONNECT_EVENT(name, methodName)						name.connect(this, &methodName);
#	define ET_CONNECT_EVENT_TO_OBJECT(name, object, methodName)		name.connect(object, &methodName);
#	define ET_CONNECT_EVENT_TO_EVENT(name, event)					name.connect(event);

	class EventReceiver;

	class Event
	{
	public:
		virtual ~Event() { }
		virtual void receiverDisconnected(EventReceiver* receiver) = 0;
	};

	class EventReceiver
	{
	public:
		virtual ~EventReceiver();
		
		void eventConnected(Event* e);
		void eventDisconnected(Event* e);

	private:
		typedef std::vector<Event*> EventList;
		EventList _events;
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
			{ return 0; }

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

	class Event0 : public Event, public Event0ConnectionBase
	{
	public:
		Event0();
		~Event0();

		template <typename R>
		void connect(R* receiver, void (R::*receiverMethod)());
		void connect(Event0& e);

		template <typename R>
		void disconnect(R* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke();
		void invokeInMainRunLoop(float delay = 0.0f);

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

	template <typename ArgType>
	class Event1 : public Event, public Event1ConnectionBase<ArgType>
	{
	public:
		Event1();
		~Event1();

		template <typename ReceiverType>
		void connect(ReceiverType* receiver, void (ReceiverType::*receiverMethod)(ArgType));
		void connect(Event1& e);

		template <typename ReceiverType>
		void disconnect(ReceiverType* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke(ArgType arg);
		void invokeInMainRunLoop(ArgType arg, float delay = 0.0f);

	private:
		void cleanup();

		EventReceiver* receiver() 
			{ return 0; }

	private:
		typedef std::vector<Event1ConnectionBase<ArgType>*> ConnectionList;
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

	template <typename Arg1Type, typename Arg2Type>
	class Event2 : public Event, public Event2ConnectionBase<Arg1Type, Arg2Type>
	{
	public:
		Event2();
		~Event2();

		template <typename ReceiverType>
		void connect(ReceiverType* receiver, void (ReceiverType::*receiverMethod)(Arg1Type, Arg2Type));
		void connect(Event2& e);

		template <typename ReceiverType>
		void disconnect(ReceiverType* receiver);

		void receiverDisconnected(EventReceiver* r);
		void invoke(Arg1Type a1, Arg2Type a2);
		void invokeInMainRunLoop(Arg1Type a1, Arg2Type a2, float delay = 0.0f);

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