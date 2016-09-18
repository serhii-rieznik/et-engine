/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/invocation.h>
#include <et/app/application.h>

using namespace et;

/*
 * Invocation Task
 */

InvocationTask::InvocationTask(PureInvocationTarget* invocation) :
	_invocation(invocation)
{
}

InvocationTask::~InvocationTask()
{
	etDestroyObject(_invocation);
}

void InvocationTask::execute() 
{
	_invocation->invoke(); 
}

/*
 * Invocation (0)
 */

void Invocation::invoke()
{
	_target->invoke();	 
}

void Invocation::invokeInMainRunLoop(float delay)
{
	invokeInRunLoop(mainRunLoop(), delay);
}

void Invocation::invokeInCurrentRunLoop(float delay)
{
	invokeInRunLoop(currentRunLoop(), delay);
}

void Invocation::invokeInBackground(float delay)
{
	invokeInRunLoop(backgroundRunLoop(), delay);
}

void Invocation::invokeInRunLoop(RunLoop& rl, float delay)
{
	rl.addTask(etCreateObject<InvocationTask>(_target->copy()), delay);
}

/*
 * Invocation (1)
 */

void Invocation1::invoke()
{ 
	_target->invoke();	 
}

void Invocation1::invokeInMainRunLoop(float delay)
{
	invokeInRunLoop(mainRunLoop(), delay);
}

void Invocation1::invokeInCurrentRunLoop(float delay)
{
	invokeInRunLoop(currentRunLoop(), delay);
}

void Invocation1::invokeInBackground(float delay)
{
	invokeInRunLoop(backgroundRunLoop(), delay);
}

void Invocation1::invokeInRunLoop(RunLoop& rl, float delay)
{
	rl.addTask(etCreateObject<InvocationTask>(_target->copy()), delay);
}

/*
 * Invocation (2)
 */

void Invocation2::invoke()
{ 
	_target->invoke();	 
}

void Invocation2::invokeInMainRunLoop(float delay)
{
	invokeInRunLoop(mainRunLoop(), delay);
}

void Invocation2::invokeInCurrentRunLoop(float delay)
{
	invokeInRunLoop(currentRunLoop(), delay);
}

void Invocation2::invokeInBackground(float delay)
{
	invokeInRunLoop(backgroundRunLoop(), delay);
}

void Invocation2::invokeInRunLoop(RunLoop& rl, float delay)
{
	rl.addTask(etCreateObject<InvocationTask>(_target->copy()), delay);
}
