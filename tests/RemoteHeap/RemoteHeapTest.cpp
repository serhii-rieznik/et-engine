#include <et/app/application.h>

const uint32_t heapCapacity = 4 * 1024 * 1024;
const uint32_t heapGranularity = 128;
const uint32_t totalAllocations = heapCapacity / heapGranularity;

void runTest()
{
	et::HeapController heap(heapCapacity, heapGranularity);
	std::vector<uint8_t> infoStorage(heap.requiredInfoSize());
	heap.setInfoStorage(infoStorage.data());

	std::vector<uint32_t> allocations;
	allocations.reserve(totalAllocations);

	uint64_t times[3] = { et::queryCurrentTimeInMicroSeconds() };

	for (uint32_t i = 0; i < totalAllocations; ++i)
	{
		uint32_t mem = 0;
		if (heap.allocate(8 + rand() % (heapGranularity - 8), mem))
			allocations.emplace_back(mem);
	}

	times[1] = et::queryCurrentTimeInMicroSeconds();

	for (uint32_t i : allocations)
		heap.release(i);

	times[2] = et::queryCurrentTimeInMicroSeconds();

	uint64_t allocTime = times[1] - times[0];
	uint64_t releaseTime = times[2] - times[1];
	uint64_t totalTime = times[2] - times[0];

	et::log::info("Times : % 4llu.%04llu | % 4llu.%04llu | % 4llu.%04llu", 
		totalTime / 1000, totalTime % 1000,
		allocTime / 1000, allocTime % 1000,
		releaseTime / 1000, releaseTime % 1000);
}

int main()
{
	et::log::addOutput(et::log::ConsoleOutput::Pointer::create());
	et::log::info("Starting test...");

	for (uint32_t i = 0; i < 5; ++i)
		runTest();

	system("pause");
	return 0;
}

et::IApplicationDelegate* et::Application::initApplicationDelegate() { return nullptr; };
