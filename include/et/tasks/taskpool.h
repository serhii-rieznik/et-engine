/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
		
	private:
		CriticalSection _csModifying;
		TaskList _tasks;
		TaskList _tasksToAdd;
		float _lastTime;
	};


}