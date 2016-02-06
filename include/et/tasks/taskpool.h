/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/tasks/tasks.h>
#include <et/threading/criticalsection.h>

namespace et
{
	class TaskPool
	{
	public:
		TaskPool();
		~TaskPool();
		
		void update(float t);
		void addTask(Task* t, float delay = 0.0f);
		
		bool hasTasks();
				
	private:
		void joinTasks();
		
		ET_DENY_COPY(TaskPool)
		
	private:
		CriticalSection _csModifying;
        Task::List _tasks;
		Task::List _tasksToAdd;
		float _lastTime = 0.0f;
	};
}
