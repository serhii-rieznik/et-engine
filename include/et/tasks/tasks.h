/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class Task
	{
	public:
		virtual ~Task()	{ }
		
		virtual void execute() = 0;

	private:
		float executionTime() const 
			{ return _executionTime; }

		void setExecutionTime(float t)
			{ _executionTime = t; }

		friend class TaskPool;

	private:
		float _executionTime = 0.0f;
	};
	
	typedef std::vector<Task*> TaskList;
}
