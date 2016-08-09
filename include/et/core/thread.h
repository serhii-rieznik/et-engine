/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <mutex>
#include <et/core/threading.h>

namespace et
{
	class Thread
	{
	public:
		Thread(const std::string& name = std::string())
			: _name(name)
		{
		}
		
		virtual ~Thread()
		{
			ET_ASSERT(!_suspended);
		}

		void run()
		{
			ET_ASSERT(!_running);
			_thread = std::thread(&Thread::threadFunction, this);
		}

		void suspend()
		{
			ET_ASSERT(!_suspended);
			_suspended = true;
			{
				std::unique_lock<std::mutex> lock(_suspendMutex);
				_suspendLock.wait(lock);
			}
			_suspended = false;
		}

		void resume()
		{
			ET_ASSERT(_suspended);
			_suspendLock.notify_all();
		}

		void stop()
		{
			_running = false;
			if (_suspended)
			{
				resume();
			}
		}

		void join()
		{
			_thread.join();
		}

		bool running() const
		{ 
			return _running; 
		}

		bool suspended() const
		{
			return _suspended; 
		}

		threading::ThreadIdentifier identifier() const
		{
			return std::hash<std::thread::id>()(_thread.get_id());
		}

		virtual void main() { }

	private:
		void setName()
		{
			if (_name.empty())
				return;

#		if (ET_PLATFORM_APPLE)
			pthread_setname_np(_name.c_str());
#		endif
		}

		void threadFunction()
		{
			_running = true;
			setName();
			main();
		}

	private:
		ET_DENY_COPY(Thread)
		Thread(Thread&&) = delete;

		std::thread _thread;
		std::condition_variable _suspendLock;
		std::mutex _suspendMutex;
		std::atomic<bool> _running{false};
		std::atomic<bool> _suspended{false};
		std::string _name;
	};
}
