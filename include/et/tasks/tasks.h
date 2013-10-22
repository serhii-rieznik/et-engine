/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class Task
	{
	protected:
		Task() : _executionTime(0.0f) { }
		virtual ~Task()	{ }

		virtual void execute() = 0;

	private:
		float executionTime() const 
			{ return _executionTime; }

		void setExecutionTime(float t)
			{ _executionTime = t; }

		friend class TaskPool;

	private:
		float _executionTime;
	};
	typedef std::list<Task*> TaskList;

	template <typename T>
	class DeletionTask : public Task
	{
	public:
		DeletionTask(T* obj) : _object(obj) { }

		void execute()
			{ delete _object; }

	private:
		T* _object;
	};
}