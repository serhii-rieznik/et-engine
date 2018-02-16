/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

namespace et {
namespace threading {

using ThreadIdentifier = uint64_t;

void setMainThreadIdentifier(ThreadIdentifier);
void setRenderThreadIdentifier(ThreadIdentifier);

ThreadIdentifier mainThreadIdentifier();
ThreadIdentifier renderThreadIdentifier();

bool inMainThread();
bool inRenderThread();

size_t maxConcurrentThreads();

ThreadIdentifier currentThread();

void sleep(float seconds);
void sleepMSec(uint64_t msec);
}
}
