/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/thread.h>
#include <et/app/runloop.h>

namespace et {

class RenderThread : public Thread
{
public:
	RenderThread();

private:
	void main() override;
};

class BackgroundThread : public Thread
{
public:
	BackgroundThread();

	RunLoop& runLoop() {
		return _runLoop;
	}

private:
	void main() override;

private:
	struct BackgroundRunLoop : public RunLoop
	{
		void addTask(Task* t, float);
		BackgroundThread* owner = nullptr;
	} _runLoop;
};

}
