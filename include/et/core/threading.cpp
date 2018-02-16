/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

static et::threading::ThreadIdentifier mainThreadId = 0;
static et::threading::ThreadIdentifier renderThreadId = 0;

et::threading::ThreadIdentifier et::threading::currentThread() {
	std::hash<std::thread::id> hasher;
	return hasher(std::this_thread::get_id());
}

void et::threading::setMainThreadIdentifier(et::threading::ThreadIdentifier t) {
	mainThreadId = t;
}

void et::threading::setRenderThreadIdentifier(et::threading::ThreadIdentifier t) {
	renderThreadId = t;
}

et::threading::ThreadIdentifier et::threading::mainThreadIdentifier() {
	ET_ASSERT(mainThreadId != 0);
	return mainThreadId;
}

et::threading::ThreadIdentifier et::threading::renderThreadIdentifier() {
	ET_ASSERT(renderThreadId != 0);
	return renderThreadId;
}

bool et::threading::inMainThread() {
	ET_ASSERT(mainThreadId != 0);
	return currentThread() == mainThreadId;
}

bool et::threading::inRenderThread() {
	ET_ASSERT(renderThreadId != 0);
	return currentThread() == renderThreadId;
}

size_t et::threading::maxConcurrentThreads() {
	return std::thread::hardware_concurrency();
}

void et::threading::sleep(float seconds) {
	sleepMSec(static_cast<uint64_t>(seconds * 1000.0f));
}

void et::threading::sleepMSec(uint64_t msec) {
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}
