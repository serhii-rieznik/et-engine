/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class Task
	{
    public:
        using List = Vector<Task*>;
        
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
}
