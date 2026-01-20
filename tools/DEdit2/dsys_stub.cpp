#include "bdefs.h"

#include <chrono>
#include <thread>

void dsi_Sleep(uint32 ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
